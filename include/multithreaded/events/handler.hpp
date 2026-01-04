#pragma once

#include <variant>

#include "events/envelope.hpp"
namespace multithreaded::events {

// This is used to determine whether a class is a valid event handler
struct any_handler {};

template <typename event_t>
class handler {
 public:
  handler() = default;
  virtual void operator()(const envelope<event_t>& val) = 0;
};

template <typename... event_ts>
class handlers : public any_handler, public handler<event_ts>... {
 public:
  using event_handler_t = handlers<event_ts...>;
  using event_receiver_t = std::variant<event_ts...>;
  using enveloped_event_generator_t = envelope_generator<event_ts...>;
  using handler<event_ts>::handler...;
  using handler<event_ts>::operator()...;
};
}  // namespace multithreaded::events
