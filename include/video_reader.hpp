#pragma once

#include <filesystem>
#include <optional>
#include <span>

#include "info.hpp"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace mvplayer {
class video_reader {
 public:
  ~video_reader() noexcept;
  [[nodiscard]] std::optional<video_info> load_video(
      const std::filesystem::path& filename) noexcept;
  [[nodiscard]] std::optional<frame_info> get_frame() const noexcept;

 private:
  [[nodiscard]] std::span<AVStream*> get_media_streams() const noexcept;
  [[nodiscard]] bool load_codec_context(
      const AVCodec* codec_ptr, AVCodecParameters* codec_params_ptr) noexcept;

 private:
  AVFormatContext* format_context_ptr_{nullptr};
  AVCodecContext* codec_context_ptr_{nullptr};
};
};  // namespace mvplayer
