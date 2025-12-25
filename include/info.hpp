#pragma once

#include <cstdint>
#include <string_view>

namespace mvplayer {
struct VideoInfo {
  std::string_view codecName, format;
  int64_t bitRate, duration;
  int32_t width, height;
};

struct FrameInfo {
  int64_t pts, dts;
};
}  // namespace mvplayer
