#pragma once

#include <opencv2/core/mat.hpp>

#include "info.hpp"

extern "C" {
#include <libavutil/rational.h>
}
namespace mvplayer::events {
struct playback_toggled {};

struct new_frame_loaded {
  cv::Mat frame;
  int64_t frame_num;
  int64_t frame_pts;
  int64_t frame_pkt_dts;
  bool reset_frame_sequence;
};

struct new_video_loaded {
  video_info info;
};

struct new_audio_samples_loaded {
  std::vector<uint8_t> samples;
  int64_t frame_num;
  int64_t frame_pts;
  int64_t frame_pkt_dts;
  bool reset_frame_sequence;
};

struct seek_request {
  seek_direction direction;
};
}  // namespace mvplayer::events
