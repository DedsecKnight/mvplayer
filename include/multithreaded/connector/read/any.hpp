#pragma once

#include "connector/read/event_port.hpp"
#include "port.hpp"

namespace multithreaded::read {
class any {
 private:
  struct port_concept {
    virtual ~port_concept() = default;

    template <typename event_t>
    [[nodiscard]] bool get_event(event_t& event) noexcept {
      auto event_port_ptr = dynamic_cast<event_port<event_t>*>(this);
      if (!event_port_ptr) {
        return false;
      }
      return event_port_ptr->get_event(event);
    }
  };

  template <typename... event_ts>
  class port_model
      : public port_concept,
        public event_port_impl<port_model<event_ts...>, event_ts>... {
   public:
    using acceptable_event_set_t = typename port<event_ts...>::queue_elem_t;
    port_model(port<event_ts...>&& rport) : read_port_{std::move(rport)} {}

   private:
    template <typename queue_elem_t>
    [[nodiscard]] bool pop(queue_elem_t& elem) noexcept {
      if constexpr ((std::is_same_v<event_ts, queue_elem_t> || ...)) {
        return read_port_.pop(elem);
      }
      return false;
    }

   private:
    port<event_ts...> read_port_;
  };

  std::unique_ptr<port_concept> pimpl_;

 public:
  template <typename... event_ts>
  any(port<event_ts...>&& rport)
      : pimpl_{std::make_unique<port_model<event_ts...>>(std::move(rport))} {}

  template <typename event_t>
  [[nodiscard]] bool get_event(event_t& e) noexcept {
    return pimpl_->get_event(e);
  }

  template <typename... event_ts>
  [[nodiscard]] port_model<event_ts...>& as_port_of() const {
    auto* model_ptr = dynamic_cast<port_model<event_ts...>*>(pimpl_.get());
    if (!model_ptr) {
      throw std::bad_cast{};
    }
    return *model_ptr;
  }
};
}  // namespace multithreaded::read
