#pragma once

#include <SDL3/SDL.h>

#include <cstdint>
#include <opencv2/core.hpp>

#include "utils/owned.hpp"

namespace mvplayer {
class FrameRenderer {
 public:
  [[nodiscard]] FrameRenderer(int32_t width, int32_t height, int32_t padding);
  [[nodiscard]] bool renderFrame(const cv::Mat& frame) const noexcept;
  void start() const noexcept;
  ~FrameRenderer();

 private:
  int32_t width_, height_, padding_;
  OwnedSdlWindow window_{nullptr};
  OwnedSdlRenderer renderer_{nullptr};
  OwnedSdlTexture texture_{nullptr};
};
}  // namespace mvplayer
