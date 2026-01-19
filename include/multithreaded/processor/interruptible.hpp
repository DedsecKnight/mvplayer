#pragma once

namespace multithreaded::processor {
class interruptible_processor {
 public:
  [[nodiscard]] virtual bool halt_requested() const noexcept = 0;
  virtual ~interruptible_processor() = default;
};
}  // namespace multithreaded::processor
