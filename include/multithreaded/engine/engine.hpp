#pragma once

#include <concepts>
#include <functional>
#include <span>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "connector/connector.hpp"
#include "engine/utils.hpp"
#include "processor/processor.hpp"
#include "utils/traits.hpp"

namespace multithreaded {

class engine {
 private:
  template <typename processor_t>
  class processor_ref {
   public:
    using type = processor_t;
    [[nodiscard]] processor_ref(
        const std::reference_wrapper<any_processor>& processor_ref,
        const std::reference_wrapper<engine>& engine_ref, std::string_view name)
        : processor_ref_{processor_ref}, engine_ref_{engine_ref}, name_{name} {}

    [[nodiscard]] any_processor& get() const noexcept {
      return processor_ref_.get();
    }

    [[nodiscard]] std::string_view name() const noexcept { return name_; }

    template <typename... event_ts>
    void subscribe_to(const auto& other) const noexcept {
      auto [read_port, write_port] =
          engine_ref_.get().create_connector<event_ts...>();

      processor_ref_.get().as<processor_t>().add_read_port(
          other.name(), std::move(read_port));

      using other_processor_t =
          typename std::remove_cvref_t<decltype(other)>::type;
      other.get().template as<other_processor_t>().add_write_port(
          name_, std::move(write_port));
    }

   private:
    std::reference_wrapper<any_processor> processor_ref_;
    std::reference_wrapper<engine> engine_ref_;
    std::string_view name_;
  };

 public:
  engine() = default;

  engine(const engine&) = delete;
  engine& operator=(const engine&) = delete;

  engine(engine&&) = delete;
  engine& operator=(engine&&) = delete;

  void start(std::span<char* const> args) noexcept;

  template <typename processor_t, typename... arg_ts>
    requires std::constructible_from<processor_t, arg_ts...> &&
             traits::is_event_handler<processor_t>
  [[nodiscard]] processor_ref<processor_t> create_processor(
      const std::string& processor_name, arg_ts&&... args) noexcept {
    // TODO: we can't pass around references like this
    auto [it, _] = processor_registry_.emplace(
        processor_name, processor_t{std::forward<arg_ts>(args)...});

    inject_system_event_writer<processor_t>(it->second);

    return processor_ref<processor_t>{std::ref(it->second), std::ref(*this),
                                      std::string_view{it->first}};
  }

  [[nodiscard]] bool terminated() const noexcept {
    return is_terminated_.load(std::memory_order_relaxed);
  }
  void terminate() noexcept {
    is_terminated_.store(true, std::memory_order_relaxed);
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

  template <typename processor_t>
  void inject_system_event_writer(any_processor& processor) {
    auto [read_port, write_port] =
        create_connector<system_events::system_terminate_request_event>();
    system_event_read_ports_.emplace_back(read_port);
    processor.as<processor_t>().add_write_port(constants::ENGINE_IDENTIFIER,
                                               std::move(write_port));
  }

  std::vector<std::thread> processor_threads_;
  std::unordered_map<std::string, any_processor> processor_registry_;
  std::vector<connector> connectors_;
  std::vector<read::port<system_events::system_terminate_request_event>>
      system_event_read_ports_;
  std::atomic<bool> is_terminated_{false};
};
}  // namespace multithreaded
