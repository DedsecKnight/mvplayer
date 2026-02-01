#pragma once

#include <array>
extern "C" {
#include <libavutil/frame.h>
}

#include <cstdint>
#include <vector>
namespace mvplayer {
struct yuv_frame {
  static constexpr size_t NUM_PLANES = 3;
  std::array<std::vector<uint8_t>, NUM_PLANES> data{};
  std::array<int, NUM_PLANES> linesize{};
};
}  // namespace mvplayer
