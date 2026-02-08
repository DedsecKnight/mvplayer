#pragma once

#include <type_traits>
namespace multithreaded::read {

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
  [[nodiscard]] virtual bool get_event(event_t& elem) noexcept = 0;
};

// TODO: figure out whether we should define move semantics and copy semantics
template <typename port_t, typename event_t>
class event_port_impl : public event_port<event_t> {  // NOLINT
 public:
  [[nodiscard]] bool get_event(event_t& event) noexcept override {
    return static_cast<port_t*>(this)->pop(event);
  }

  virtual ~event_port_impl() = default;
};
};  // namespace multithreaded::read
