#pragma once

#include <variant>
namespace multithreaded::events {
template <typename event_t>
class handler {
 public:
  handler() = default;
  virtual void operator()(const event_t& val) = 0;
};

template <typename... event_ts>
class handlers : public handler<event_ts>... {
 public:
  using event_handler_t = handlers<event_ts...>;
  using event_receiver_t = std::variant<event_ts...>;
  using handler<event_ts>::handler...;
  using handler<event_ts>::operator()...;
};
}  // namespace multithreaded::events
