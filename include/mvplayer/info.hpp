#pragma once

#include <cstdint>
#include <string_view>

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
}

namespace mvplayer {
struct media_info {
  std::string_view format;
  AVRational tbn;
};

struct picture_info : public media_info {
  AVRational fps;
  int32_t width, height;
};

struct audio_info : public media_info {
  AVChannelLayout* channel_layout;
  AVSampleFormat sample_format;
  int32_t num_channels, frequency;
  bool has_audio_stream;
};

struct video_info {
  std::string_view codec_name;
  picture_info picture;
  audio_info audio;
};

enum class seek_direction : uint8_t { forward, backward };

}  // namespace mvplayer
