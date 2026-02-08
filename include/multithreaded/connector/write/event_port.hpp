#pragma once

#include <type_traits>
namespace multithreaded::write {
class event_port_wrapper {
 public:
  template <typename T>
    requires std::is_base_of_v<event_port_wrapper, T>
  constexpr T* as_event_port_of() {
    return static_cast<T*>(this);
  }
};

template <typename event_t>
class event_port : public event_port_wrapper {  // NOLINT
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
