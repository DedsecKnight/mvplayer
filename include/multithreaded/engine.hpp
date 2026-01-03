#pragma once

#include <concepts>
#include <flat_map>
#include <functional>
#include <span>
#include <thread>
#include <utility>
#include <vector>

#include "connector/connector.hpp"
#include "processor/processor.hpp"

namespace multithreaded {
class engine {
 private:
  using processor_name_t = std::string_view;

  template <typename processor_t>
  class processor_ref {
   public:
    using type = processor_t;
    [[nodiscard]] processor_ref(const std::reference_wrapper<any_processor>& p,
                                const std::reference_wrapper<engine>& e)
        : p_{p}, e_{e} {}

    any_processor& get() const noexcept { return p_.get(); }

    template <typename... event_ts>
    void subscribe_to(const auto& other) const noexcept {
      auto [read_port, write_port] = e_.get().create_connector<event_ts...>();

      p_.get().as<processor_t>().add_read_port(std::move(read_port));

      using other_processor_t =
          typename std::remove_cvref_t<decltype(other)>::type;
      other.get().template as<other_processor_t>().add_write_port(
          std::move(write_port));
    }

   private:
    std::reference_wrapper<any_processor> p_;
    std::reference_wrapper<engine> e_;
  };

 public:
  void start(std::span<char* const> args) noexcept;

  // TODO: add constraint to allow only event handler as processor_t
  template <typename processor_t, typename... arg_ts>
    requires std::constructible_from<processor_t, arg_ts...>
  [[nodiscard]] processor_ref<processor_t> create_processor(
      processor_name_t processor_name, arg_ts&&... args) noexcept {
    processor_registry_.emplace(processor_name,
                                processor_t{std::forward<arg_ts>(args)...});

    return processor_ref<processor_t>{
        std::ref(processor_registry_.at(processor_name)), std::ref(*this)};
  }

  ~engine() noexcept;

 private:
  template <typename... event_ts>
  [[nodiscard]] decltype(auto) create_connector() noexcept {
    connectors_.emplace_back(connector_event_set<event_ts...>{});
    auto& event_connector = connectors_.back().as_connector_of<event_ts...>();
    return std::make_pair(event_connector.get_read_port(),
                          event_connector.get_write_port());
  }

 private:
  std::vector<std::thread> processor_threads_;
  std::flat_map<processor_name_t, any_processor> processor_registry_;
  std::vector<connector> connectors_;
};
}  // namespace multithreaded
