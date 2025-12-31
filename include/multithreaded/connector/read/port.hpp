#pragma once

#include <variant>

#include "utils/queue.hpp"

namespace multithreaded::read {
template <typename... event_ts>
class port {
 private:
  using queue_t = utils::spsc_queue<std::variant<event_ts...>>;

 public:
  using queue_elem_t = typename queue_t::elem_t;

  port(const std::reference_wrapper<queue_t>& queue_ref) : queue_{queue_ref} {}

  [[nodiscard]] bool pop(queue_elem_t& elem) noexcept {
    return queue_.get().pop(elem);
  }

 private:
  std::reference_wrapper<queue_t> queue_;
};

}  // namespace multithreaded::read
