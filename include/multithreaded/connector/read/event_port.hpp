#pragma once

#include <variant>

namespace multithreaded::read {
template <typename event_t>
class event_port {
 public:
  virtual ~event_port() = default;
  virtual bool get_event(event_t& elem) noexcept = 0;
};

template <typename port_t, typename event_t>
class event_port_impl : public event_port<event_t> {
 public:
  bool get_event(event_t& event) noexcept override {
    using queue_elem_t = typename port_t::acceptable_event_set_t;
    queue_elem_t elem;
    if (!static_cast<port_t*>(this)->pop(elem)) {
      return false;
    }
    event = std::get<event_t>(elem);
    return true;
  }

  virtual ~event_port_impl() = default;
};
};  // namespace multithreaded::read
