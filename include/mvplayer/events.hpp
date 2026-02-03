#pragma once

#include "info.hpp"

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/rational.h>
}
namespace mvplayer::events {
struct playback_toggled {};

struct new_frame_loaded {
  AVFrame* frame;
  int64_t frame_num;
  int64_t frame_pts;
  bool reset_frame_sequence;
};

struct new_video_loaded {
  video_info info;
};

struct new_audio_samples_loaded {
  AVFrame* frame;
  int64_t frame_num;
  int32_t num_channels;
  bool reset_frame_sequence;
};

struct seek_request {
  seek_direction direction;
};
}  // namespace mvplayer::events
