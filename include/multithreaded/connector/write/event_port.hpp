#pragma once

namespace multithreaded::write {
template <typename event_t>
class event_port {  // NOLINT
 public:
  virtual ~event_port() = default;
  [[nodiscard]] virtual bool send_event(const event_t& event) noexcept = 0;
};

template <typename port_t, typename event_t>
class event_port_impl : public event_port<event_t> {
  [[nodiscard]] bool send_event(const event_t& event) noexcept override {
    return static_cast<port_t*>(this)->push(event);
  }
};
}  // namespace multithreaded::write
