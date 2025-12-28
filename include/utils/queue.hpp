#pragma once

#include <spdlog/spdlog.h>

#include <atomic>
#include <bit>
#include <cstddef>
#include <memory>

namespace mvplayer::utils {

/// Single Producer - Single Consumer Lock-free Ring Buffer
template <typename T, std::size_t Size = 3>
  requires(std::has_single_bit(Size + 1))
class SPSCQueue {
 private:
  static constexpr size_t CACHE_LINE_SIZE = 64;

 public:
  [[nodiscard]] bool push(const T& elem) noexcept { return emplace(elem); }

  template <typename... ArgTs>
    requires std::constructible_from<T, ArgTs...>
  [[nodiscard]] bool emplace(ArgTs&&... args) noexcept {
    auto currentWriteIndex = writeIndex_.load(std::memory_order_relaxed);
    if (currentWriteIndex - readIndex_.load(std::memory_order_acquire) >=
        Size) {
      spdlog::trace("Queue is full");
      return false;
    }

    std::construct_at(
        reinterpret_cast<T*>(data_ + sizeof(T) * (currentWriteIndex & Size)),
        std::forward<ArgTs>(args)...);

    writeIndex_.store(currentWriteIndex + 1, std::memory_order_release);
    return true;
  }

  [[nodiscard]] bool pop(T& elem) noexcept {
    auto currentReadIndex = readIndex_.load(std::memory_order_relaxed);
    if (currentReadIndex == writeIndex_.load(std::memory_order_acquire)) {
      spdlog::trace("Queue is empty");
      return false;
    }

    T* queueElement =
        reinterpret_cast<T*>(data_ + sizeof(T) * (currentReadIndex & Size));
    std::construct_at(&elem, *queueElement);
    queueElement->~T();

    readIndex_.store(currentReadIndex + 1, std::memory_order_release);
    return true;
  }

 private:
  alignas(CACHE_LINE_SIZE) std::atomic<size_t> readIndex_{0};
  alignas(CACHE_LINE_SIZE) std::atomic<size_t> writeIndex_{0};
  alignas(CACHE_LINE_SIZE) std::byte data_[sizeof(T) * (Size + 1)];
};

}  // namespace mvplayer::utils
