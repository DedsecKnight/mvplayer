#pragma once

namespace multithreaded::processor {
class termination_handler {
 public:
  termination_handler() = default;

  termination_handler(const termination_handler&) = default;
  termination_handler& operator=(const termination_handler&) = default;

  termination_handler(termination_handler&&) = default;
  termination_handler& operator=(termination_handler&&) = default;

  virtual ~termination_handler() = default;
  virtual void handle_termination_signal() noexcept = 0;
};
}  // namespace multithreaded::processor
