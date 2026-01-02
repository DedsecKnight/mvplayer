#pragma once

#include "port.hpp"
#include "utils/traits.hpp"

namespace multithreaded::read {
class any {
 private:
  struct port_concept {
    virtual ~port_concept() = default;

    // template <typename event_t>
    // [[nodiscard]] std::optional<event_t> get_next() const noexcept {
    //   return static_cast<event_port<event_t>*>(this)->get();
    // }
  };

  template <typename event_t>
  struct event_port {
    [[nodiscard]] std::optional<event_t> get() const noexcept {
      return static_cast<port_accessor>
    }
  };

  // template <typename model_t>
  // struct port_accessor {
  //   template <typename event_t>
  //   [[nodiscard]] std::optional<event_t> get_next_impl() const noexcept {
  //     using possible_event_set_t = typename model_t::acceptable_event_set_t;
  //     if constexpr (utils::variant_holds_alternative_v<event_t,
  //                                                      possible_event_set_t>)
  //                                                      {
  //       possible_event_set_t e;
  //       if (!static_cast<model_t*>(this)->pop(e)) {
  //         return std::nullopt;
  //       }
  //
  //       return std::get<event_t>(e);
  //     } else {
  //       return std::nullopt;
  //     }
  //   }
  // };

  template <typename... event_ts>
  struct port_accessor : public event_port<event_ts>... {};

  template <typename... event_ts>
  class port_model : public port_concept, public port_accessor<event_ts...> {
   public:
    using acceptable_event_set_t = typename port<event_ts...>::queue_elem_t;
    port_model(port<event_ts...>&& rport) : read_port_{std::move(rport)} {}

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
