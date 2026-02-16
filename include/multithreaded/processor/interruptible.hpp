#pragma once

namespace multithreaded::processor {
class interruptible_processor {
 public:
  interruptible_processor() = default;

  interruptible_processor(const interruptible_processor&) = default;
  interruptible_processor& operator=(const interruptible_processor&) = default;

  interruptible_processor(interruptible_processor&&) = default;
  interruptible_processor& operator=(interruptible_processor&&) = default;

  [[nodiscard]] virtual bool halt_requested() const noexcept = 0;
  virtual ~interruptible_processor() = default;
};
}  // namespace multithreaded::processor
