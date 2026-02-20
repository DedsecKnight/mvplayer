#pragma once

#include <queue>
#include <typeindex>
#include <variant>

#include "connector/read/event_port.hpp"
#include "port.hpp"

namespace multithreaded::read {
class any {
 private:
  class container {  // NOLINT
   public:
    virtual ~container() = default;

    template <typename... processor_event_ts>
    [[nodiscard]] constexpr bool get_event(
        std::variant<processor_event_ts...>& event_holder) noexcept {
      return (event_fetcher(event_holder, processor_event_ts{}) || ...);
    }

   private:
    template <typename event_t, typename... processor_event_ts>
    bool event_fetcher(std::variant<processor_event_ts...>& event_holder,
                       event_t event) {
      using processor_event_t = std::decay_t<event_t>;
      namespace ranges = std::ranges;

      std::type_index type_id{typeid(processor_event_t)};

      if (ranges::find(invalid_types_, type_id) != std::cend(invalid_types_)) {
        return false;
      }

      const auto port_it = ranges::find_if(
          casted_port_mapper_,
          [&type_id](const auto& port) { return port.first == type_id; });
      bool port_cached = port_it != std::cend(casted_port_mapper_);
      auto event_port_ptr =
          port_cached
              ? port_it->second
                    ->template as_event_port_of<event_port<processor_event_t>>()
              : dynamic_cast<event_port<processor_event_t>*>(this);

      if (!event_port_ptr) {
        invalid_types_.push_back(type_id);
        return false;
      }

      if (!port_cached) {
        casted_port_mapper_.emplace_back(type_id, event_port_ptr);
      }

      if (!event_port_ptr->get_event(event)) {
        return false;
      }

      event_holder = std::move(event);
      return true;
    }

    std::vector<std::type_index> invalid_types_;
    std::vector<std::pair<std::type_index, event_port_wrapper*>>
        casted_port_mapper_;
  };

  template <typename... event_ts>
  class model : public container,
                public event_port_impl<model<event_ts...>, event_ts>... {
   public:
    using queue_elem_t = typename port<event_ts...>::queue_elem_t;
    explicit model(port<event_ts...>&& rport) : read_port_{std::move(rport)} {}

    template <typename event_t>
    [[nodiscard]] bool pop(event_t& elem) noexcept {
      if (!pending_queue_.empty() &&
          std::holds_alternative<event_t>(pending_queue_.front())) {
        elem = std::move(std::get<event_t>(pending_queue_.front()));
        pending_queue_.pop();
        return true;
      }
      queue_elem_t queue_element;
      if (!read_port_.pop(queue_element)) {
        return false;
      }
      if (std::holds_alternative<event_t>(queue_element)) {
        elem = std::move(std::get<event_t>(queue_element));
        return true;
      }
      pending_queue_.emplace(std::move(queue_element));
      return false;
    }

   private:
    // TODO: use fixed-size bounded queue here
    std::queue<queue_elem_t> pending_queue_;
    port<event_ts...> read_port_;
  };

  std::unique_ptr<container> pimpl_;

 public:
  template <typename... event_ts>
  explicit any(port<event_ts...>&& rport)
      : pimpl_{std::make_unique<model<event_ts...>>(std::move(rport))} {}

  template <typename... processor_event_ts>
  [[nodiscard]] bool get_event(
      std::variant<processor_event_ts...>& event_holder) noexcept {
    return pimpl_->get_event(event_holder);
  }

  template <typename... event_ts>
  [[nodiscard]] model<event_ts...>& as_port_of() const {
    auto* model_ptr = dynamic_cast<model<event_ts...>*>(pimpl_.get());
    if (!model_ptr) {
      throw std::bad_cast{};
    }
    return *model_ptr;
  }
};
}  // namespace multithreaded::read
