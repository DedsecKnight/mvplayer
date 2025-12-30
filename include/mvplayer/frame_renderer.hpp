#pragma once

#include <SDL3/SDL.h>

#include <cstdint>
#include <opencv2/core.hpp>

#include "utils/owned.hpp"

namespace mvplayer {
class frame_renderer {
 public:
  [[nodiscard]] frame_renderer(int32_t width, int32_t height, int32_t padding);
  [[nodiscard]] bool render_frame(const cv::Mat& frame) const noexcept;
  void start() const noexcept;
  ~frame_renderer();

 private:
  int32_t width_, height_, padding_;
  sdl_window window_{nullptr};
  sdl_renderer renderer_{nullptr};
  sdl_texture texture_{nullptr};
};
}  // namespace mvplayer
