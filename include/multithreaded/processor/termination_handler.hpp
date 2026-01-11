#pragma once

namespace multithreaded::processor {
class termination_handler {
 public:
  virtual ~termination_handler() = default;
  virtual void handle_termination_signal() noexcept = 0;
};
}  // namespace multithreaded::processor
