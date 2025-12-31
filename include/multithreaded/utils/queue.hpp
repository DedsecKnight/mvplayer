#pragma once

#include <spdlog/spdlog.h>

#include <atomic>
#include <bit>
#include <cstddef>
#include <memory>

namespace multithreaded::utils {

/// Single Producer - Single Consumer Lock-free Ring Buffer
template <typename T, std::size_t Size = 3>
  requires(std::has_single_bit(Size + 1))
class spsc_queue {
 private:
  static constexpr size_t CACHE_LINE_SIZE = 64;

 public:
  using elem_t = T;

  [[nodiscard]] bool push(const T& elem) noexcept { return emplace(elem); }

  template <typename... ArgTs>
    requires std::constructible_from<T, ArgTs...>
  [[nodiscard]] bool emplace(ArgTs&&... args) noexcept {
    auto current_write_index = write_index_.load(std::memory_order_relaxed);
    if (current_write_index - read_index_.load(std::memory_order_acquire) >=
        Size) {
      spdlog::trace("Queue is full");
      return false;
    }

    std::construct_at(
        reinterpret_cast<T*>(data_ + sizeof(T) * (current_write_index & Size)),
        std::forward<ArgTs>(args)...);

    write_index_.store(current_write_index + 1, std::memory_order_release);
    return true;
  }

  [[nodiscard]] bool pop(T& elem) noexcept {
    auto current_read_index = read_index_.load(std::memory_order_relaxed);
    if (current_read_index == write_index_.load(std::memory_order_acquire)) {
      spdlog::trace("Queue is empty");
      return false;
    }

    T* queue_element =
        reinterpret_cast<T*>(data_ + sizeof(T) * (current_read_index & Size));
    std::construct_at(&elem, *queue_element);
    queue_element->~T();

    read_index_.store(current_read_index + 1, std::memory_order_release);
    return true;
  }

 private:
  alignas(CACHE_LINE_SIZE) std::atomic<size_t> read_index_{0};
  alignas(CACHE_LINE_SIZE) std::atomic<size_t> write_index_{0};
  alignas(CACHE_LINE_SIZE) std::byte data_[sizeof(T) * (Size + 1)];
};

}  // namespace multithreaded::utils
