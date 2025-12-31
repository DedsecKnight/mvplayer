#pragma once

#include "port.hpp"

namespace multithreaded::read {
class any {
 private:
  struct port_concept {
    virtual ~port_concept() = default;
  };

  template <typename... event_ts>
  class port_model : public port_concept {
   public:
    port_model(port<event_ts...>&& rport) : read_port_{std::move(rport)} {}

    template <typename queue_elem_t>
    [[nodiscard]] bool pop(queue_elem_t& elem) noexcept {
      if constexpr ((std::is_same_v<event_ts, queue_elem_t> || ...)) {
        return read_port_.pop(elem);
      }
      return false;
    }

    template <typename processor_t>
    void process_incoming_event(processor_t& receiver) {
      using event_t = typename decltype(read_port_)::queue_elem_t;
      event_t e;
      if (!read_port_.pop(e)) {
        return;
      }
      receiver.handle_event(e);
    }

   private:
    port<event_ts...> read_port_;
  };

  std::unique_ptr<port_concept> pimpl_;

 public:
  template <typename... event_ts>
  any(port<event_ts...>&& rport)
      : pimpl_{std::make_unique<port_model<event_ts...>>(std::move(rport))} {}

  // void process_incoming_event(any_processor& receiver) {}
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
