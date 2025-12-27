#pragma once

#include <SDL3/SDL.h>

#include <cstdint>
#include <opencv2/core.hpp>

namespace mvplayer {
class Renderer {
 public:
  [[nodiscard]] Renderer(int32_t width, int32_t height, int padding);
  [[nodiscard]] bool renderFrame(const cv::Mat& frame) const noexcept;
  void start() const noexcept;
  ~Renderer();

 private:
  int32_t width_, height_, padding_;
  SDL_Window* window_{nullptr};
  SDL_Renderer* renderer_{nullptr};
  SDL_Texture* texture_{nullptr};
};
}  // namespace mvplayer
