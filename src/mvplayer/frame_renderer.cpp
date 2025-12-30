#include "frame_renderer.hpp"

#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>

#include <format>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "sdl_manager.hpp"

namespace mvplayer {
frame_renderer::frame_renderer(int32_t width, int32_t height, int32_t padding)
    : width_{width}, height_{height}, padding_{padding} {
  int32_t padded_width = width_ + 2 * padding_;
  int32_t padded_height = height_ + 2 * padding_;

  window_ =
      sdl_manager::create_window("video-player", padded_width, padded_height);
  if (window_ == nullptr) {
    throw std::runtime_error{
        std::format("Error creating window: {}", SDL_GetError())};
  }
  spdlog::trace("SDL Window created successfully");

  renderer_ = sdl_manager::create_renderer(window_.get());
  if (renderer_ == nullptr) {
    throw std::runtime_error{
        std::format("Error creating renderer: {}", SDL_GetError())};
  }
  spdlog::trace("SDL Renderer created successfully");

  texture_ = sdl_manager::create_texture(
      renderer_.get(), SDL_PixelFormat::SDL_PIXELFORMAT_RGB24,
      SDL_TextureAccess::SDL_TEXTUREACCESS_STREAMING, padded_width,
      padded_height);
  if (texture_ == nullptr) {
    throw std::runtime_error{
        std::format("Error creating texture: {}", SDL_GetError())};
  }
  spdlog::trace("SDL Texture created successfully");
}

bool frame_renderer::render_frame(const cv::Mat& frame) const noexcept {
  SDL_SetRenderDrawColor(renderer_.get(), 0, 0, 0, 0);
  SDL_RenderClear(renderer_.get());

  cv::Mat padded_frame;
  cv::copyMakeBorder(frame, padded_frame, padding_, padding_, padding_,
                     padding_, cv::BORDER_CONSTANT);

  void* pixels;
  int32_t pitch;
  if (!SDL_LockTexture(texture_.get(), nullptr, &pixels, &pitch)) {
    spdlog::error("Error creating texture: {}", SDL_GetError());
    return false;
  }
  std::memcpy(pixels, padded_frame.data, frame.elemSize() * frame.total());
  SDL_UnlockTexture(texture_.get());

  if (!SDL_RenderTexture(renderer_.get(), texture_.get(), nullptr, nullptr) ||
      !SDL_RenderPresent(renderer_.get())) {
    spdlog::error("Error rendering frame: {}", SDL_GetError());
    return false;
  }
  return true;
}

frame_renderer::~frame_renderer() { SDL_QuitSubSystem(SDL_INIT_VIDEO); }
}  // namespace mvplayer
