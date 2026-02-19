#pragma once

#include <chrono>
#include <cstddef>

using namespace std::chrono_literals;

namespace multithreaded::constants {
static constexpr size_t DEFAULT_QUEUE_SIZE = 3;
static constexpr std::chrono::milliseconds SYSTEM_SIGNAL_POLL_INTERVAL = 300ms;
}  // namespace multithreaded::constants
