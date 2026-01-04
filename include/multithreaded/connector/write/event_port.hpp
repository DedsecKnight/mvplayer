#pragma once

namespace multithreaded::write {
template <typename event_t>
class event_port {
 public:
  virtual ~event_port() = default;
  virtual bool send_event(const event_t& e) noexcept = 0;
};

template <typename port_t, typename event_t>
class event_port_impl : public event_port<event_t> {
  bool send_event(const event_t& e) noexcept override {
    return static_cast<port_t*>(this)->push(e);
  }
};
}  // namespace multithreaded::write
