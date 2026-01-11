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
  template <typename event_t>
  [[nodiscard]] bool push(const event_t& elem) noexcept {
    if constexpr ((std::is_same_v<event_t, event_ts> || ...)) {
      queue_elem_holder_ = elem;
      return queue_.get().push(queue_elem_holder_);
    } else {
      return false;
    }
  }

  template <typename... arg_ts>
    requires std::constructible_from<queue_elem_t, arg_ts...>
  [[nodiscard]] bool emplace(arg_ts&&... args) noexcept {
    return queue_.get().emplace(std::forward<arg_ts>(args)...);
  }

  explicit port(const std::reference_wrapper<queue_t>& queue_ref)
      : queue_{queue_ref} {}

 private:
  queue_elem_t queue_elem_holder_;
  std::reference_wrapper<queue_t> queue_;
};
}  // namespace multithreaded::write
