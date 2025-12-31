
#pragma once

#include "port.hpp"

namespace multithreaded::write {
class any {
 private:
  struct port_concept {
    virtual ~port_concept() = default;
  };

  template <typename... event_ts>
  class port_model : public port_concept {
   public:
    port_model(port<event_ts...>&& wport) : write_port_{std::move(wport)} {}

    template <typename queue_elem_t>
    [[nodiscard]] bool push(const queue_elem_t& elem) noexcept {
      if constexpr ((std::is_same_v<event_ts, queue_elem_t> || ...)) {
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

  std::unique_ptr<port_concept> pimpl_;

 public:
  template <typename... event_ts>
  any(port<event_ts...>&& wport)
      : pimpl_{std::make_unique<port_model<event_ts...>>(std::move(wport))} {}
};
}  // namespace multithreaded::write
