#pragma once

#include <filesystem>
#include <optional>

#include "info.hpp"
extern "C" {
#include <libavformat/avformat.h>
}

namespace mvplayer {
class VideoPlayer {
 public:
  ~VideoPlayer() noexcept;
  [[nodiscard]] std::optional<VideoInfo> loadVideo(
      const std::filesystem::path& filename) noexcept;

 private:
  AVFormatContext* pFormatContext_{nullptr};
};
};  // namespace mvplayer
