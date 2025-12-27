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
class VideoReader {
 public:
  ~VideoReader() noexcept;
  [[nodiscard]] std::optional<VideoInfo> loadVideo(
      const std::filesystem::path& filename) noexcept;
  [[nodiscard]] std::optional<FrameInfo> getFrame() const noexcept;

 private:
  [[nodiscard]] std::span<AVStream*> getMediaStreams() const noexcept;
  [[nodiscard]] bool loadCodecContext(const AVCodec* pCodec,
                                      AVCodecParameters* pCodecParams) noexcept;

 private:
  AVFormatContext* pFormatContext_{nullptr};
  AVCodecContext* pCodecContext_{nullptr};
};
};  // namespace mvplayer
