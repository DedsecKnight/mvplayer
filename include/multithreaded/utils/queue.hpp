#pragma once

#include <spdlog/spdlog.h>

#include <atomic>
#include <bit>
#include <chrono>
#include <cstddef>
#include <memory>

namespace multithreaded::utils {

namespace queue_details {
static constexpr size_t DEFAULT_QUEUE_SIZE = 7;
static constexpr int32_t DEFAULT_WAIT_TIME_MS = 512;
}  // namespace queue_details

/// Single Producer - Single Consumer Lock-free Ring Buffer
template <typename T, std::size_t Size = queue_details::DEFAULT_QUEUE_SIZE>
  requires(std::has_single_bit(Size + 1))
class spsc_queue {
 private:
  static constexpr size_t CACHE_LINE_SIZE = 64;

 public:
  using elem_t = T;

  spsc_queue() : data_(sizeof(T) * (Size + 1)) {}

  [[nodiscard]] bool push(const T& elem) noexcept { return emplace(elem); }

  template <typename... ArgTs>
    requires std::constructible_from<T, ArgTs...>
  [[nodiscard]] bool emplace(ArgTs&&... args) noexcept {
    auto current_write_index = write_index_.load(std::memory_order_relaxed);
    auto next_write_index = (current_write_index + 1) & Size;
    int64_t wait_time_ms = 1;
    while (next_write_index == read_index_.load(std::memory_order_acquire)) {
      if (wait_time_ms >= queue_details::DEFAULT_WAIT_TIME_MS) {
        return false;
      }
      auto target_time_ms =
          wait_time_ms +
          std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::high_resolution_clock::now().time_since_epoch())
              .count();
      while (true) {
        auto current_time_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch())
                .count();
        if (current_time_ms >= target_time_ms) {
          break;
        }
      }
      wait_time_ms *= 2;
    }

    // NOLINTBEGIN
    std::construct_at(
        std::launder(reinterpret_cast<T*>(
            data_.data() + (sizeof(T) * (current_write_index & Size)))),
        std::forward<ArgTs>(args)...);
    // NOLINTEND

    write_index_.store(next_write_index, std::memory_order_release);
    return true;
  }

  [[nodiscard]] bool pop(T& elem) noexcept {
    auto current_read_index = read_index_.load(std::memory_order_relaxed);
    if (current_read_index == write_index_.load(std::memory_order_acquire)) {
      spdlog::trace("Queue is empty");
      return false;
    }

    // NOLINTBEGIN
    T* queue_element = std::launder(reinterpret_cast<T*>(
        data_.data() + (sizeof(T) * (current_read_index & Size))));
    // NOLINTEND

    std::construct_at(&elem, *queue_element);
    queue_element->~T();

    read_index_.store((current_read_index + 1) & Size,
                      std::memory_order_release);
    return true;
  }

 private:
  alignas(CACHE_LINE_SIZE) std::atomic<size_t> read_index_{0};
  alignas(CACHE_LINE_SIZE) std::atomic<size_t> write_index_{0};
  alignas(CACHE_LINE_SIZE) std::vector<std::byte> data_;
};

}  // namespace multithreaded::utils
