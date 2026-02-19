#pragma once

#include <chrono>
#include <cstddef>

using namespace std::chrono_literals;

namespace mvplayer::constants {
static constexpr size_t CACHE_LINE_SIZE = 64;
static constexpr std::chrono::milliseconds POLL_INTERVAL = 14ms;
}  // namespace mvplayer::constants
