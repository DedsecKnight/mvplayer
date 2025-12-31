#pragma once

#include <variant>

#include "utils/queue.hpp"

namespace multithreaded::write {

template <typename... event_ts>
class port {
 private:
  using queue_t = utils::spsc_queue<std::variant<event_ts...>>;
  using queue_elem_t = typename queue_t::elem_t;

 public:
  [[nodiscard]] bool push(const queue_elem_t& elem) noexcept {
    return queue_.get().push(elem);
  }

  template <typename... arg_ts>
    requires std::constructible_from<queue_elem_t, arg_ts...>
  [[nodiscard]] bool emplace(arg_ts&&... args) noexcept {
    return queue_.get().emplace(std::forward<arg_ts>(args)...);
  }

  port(const std::reference_wrapper<queue_t>& queue_ref) : queue_{queue_ref} {}

 private:
  std::reference_wrapper<queue_t> queue_;
};
}  // namespace multithreaded::write
