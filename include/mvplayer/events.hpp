#pragma once

#include <opencv2/core/mat.hpp>

#include "info.hpp"

extern "C" {
#include <libavutil/rational.h>
}
namespace mvplayer::events {
struct video_played {};

struct video_paused {};

struct new_frame_loaded {
  cv::Mat frame;
  int64_t frame_num;
  int64_t frame_pts;
  int64_t frame_pkt_dts;
};

struct new_video_loaded {
  video_info info;
};
}  // namespace mvplayer::events
