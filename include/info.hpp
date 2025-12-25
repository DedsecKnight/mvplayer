#pragma once

#include <cstdint>
#include <opencv2/core.hpp>
#include <string_view>

extern "C" {
#include <libavutil/avutil.h>
}

namespace mvplayer {
struct VideoInfo {
  std::string_view codecName, format;
  AVRational fps;
  int64_t bitRate, duration;
  int32_t width, height;
};

struct FrameInfo {
  cv::Mat frame;
  int64_t frameNo, pts, dts;
};
}  // namespace mvplayer
