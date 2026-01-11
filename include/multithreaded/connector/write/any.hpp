
#pragma once

#include "connector/write/event_port.hpp"
#include "port.hpp"

namespace multithreaded::write {
class any {
 private:
  struct container {  // NOLINT
    virtual ~container() = default;

    template <typename event_t>
    [[nodiscard]] bool send_event(const event_t& event) noexcept {
      auto event_port_ptr = dynamic_cast<event_port<event_t>*>(this);
      if (!event_port_ptr) {
        return false;
      }
      return event_port_ptr->send_event(event);
    }
  };

  template <typename... event_ts>
  class model : public container,
                public event_port_impl<model<event_ts...>, event_ts>... {
   public:
    explicit model(port<event_ts...>&& wport) : write_port_{std::move(wport)} {}

    template <typename event_t>
    [[nodiscard]] bool push(const event_t& elem) noexcept {
      if constexpr ((std::is_same_v<event_ts, event_t> || ...)) {
        return write_port_.push(elem);
      }
      return false;
    }

    template <typename queue_elem_t, typename... arg_ts>
      requires std::constructible_from<queue_elem_t, arg_ts...>
    [[nodiscard]] bool emplace(arg_ts&&... args) noexcept {
      if constexpr ((std::is_same_v<event_ts, queue_elem_t> || ...)) {
        return write_port_.emplace(std::forward<event_ts>(args)...);
      }
      return false;
    }

   private:
    port<event_ts...> write_port_;
  };

  std::unique_ptr<container> pimpl_;

 public:
  template <typename... event_ts>
  explicit any(port<event_ts...>&& wport)
      : pimpl_{std::make_unique<model<event_ts...>>(std::move(wport))} {}

  template <typename event_t>
  [[nodiscard]] bool send_event(const event_t& event) noexcept {
    return pimpl_->send_event(event);
  }
};
}  // namespace multithreaded::write
