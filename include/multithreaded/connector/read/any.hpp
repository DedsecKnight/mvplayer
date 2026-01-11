#pragma once

#include <queue>
#include <variant>

#include "connector/read/event_port.hpp"
#include "port.hpp"

namespace multithreaded::read {
class any {
 private:
  struct container {  // NOLINT
    virtual ~container() = default;

    template <typename... processor_event_ts>
    [[nodiscard]] bool get_event(
        std::variant<processor_event_ts...>& event_holder) noexcept {
      auto event_fetcher = [this, &event_holder](auto&& event) -> bool {
        using processor_event_t = std::decay_t<decltype(event)>;
        auto event_port_ptr =
            dynamic_cast<event_port<processor_event_t>*>(this);
        if (!event_port_ptr) {
          return false;
        }
        if (!event_port_ptr->get_event(event)) {
          return false;
        }
        event_holder = event;
        return true;
      };

      return (event_fetcher(processor_event_ts{}) || ...);
    }
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
        elem = std::get<event_t>(pending_queue_.front());
        pending_queue_.pop();
        return true;
      }
      queue_elem_t queue_element;
      if (!read_port_.pop(queue_element)) {
        return false;
      }
      if (std::holds_alternative<event_t>(queue_element)) {
        elem = std::get<event_t>(queue_element);
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
