#pragma once

#include <cstdint>
#include <opencv2/core.hpp>
#include <string_view>

extern "C" {
#include <libavutil/avutil.h>
}

namespace mvplayer {
struct video_info {
  std::string_view codec_name, format;
  AVRational fps;
  int64_t bit_rate, duration;
  int32_t width, height;
};

struct frame_info {
  cv::Mat frame;
  int64_t frame_no, pts, dts;
};
}  // namespace mvplayer
