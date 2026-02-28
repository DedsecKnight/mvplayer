#pragma once

namespace multithreaded::processor {
class pre_event_handler {
 public:
  pre_event_handler() = default;

  pre_event_handler(const pre_event_handler&) = default;
  pre_event_handler& operator=(const pre_event_handler&) = default;

  pre_event_handler(pre_event_handler&&) = default;
  pre_event_handler& operator=(pre_event_handler&&) = default;

  virtual ~pre_event_handler() = default;
  virtual void pre_event() noexcept = 0;
};
}  // namespace multithreaded::processor
