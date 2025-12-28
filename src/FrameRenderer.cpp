#include "FrameRenderer.hpp"

#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>

#include <format>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "SdlManager.hpp"

namespace mvplayer {
FrameRenderer::FrameRenderer(int32_t width, int32_t height, int32_t padding)
    : width_{width}, height_{height}, padding_{padding} {
  int32_t paddedWidth = width_ + 2 * padding_;
  int32_t paddedHeight = height_ + 2 * padding_;

  window_ = SdlManager::CreateWindow("video-player", paddedWidth, paddedHeight);
  if (window_ == nullptr) {
    throw std::runtime_error{
        std::format("Error creating window: {}", SDL_GetError())};
  }
  spdlog::trace("SDL Window created successfully");

  renderer_ = SdlManager::CreateRenderer(window_.get());
  if (renderer_ == nullptr) {
    throw std::runtime_error{
        std::format("Error creating renderer: {}", SDL_GetError())};
  }
  spdlog::trace("SDL Renderer created successfully");

  texture_ = SdlManager::CreateTexture(
      renderer_.get(), SDL_PixelFormat::SDL_PIXELFORMAT_RGB24,
      SDL_TextureAccess::SDL_TEXTUREACCESS_STREAMING, paddedWidth,
      paddedHeight);
  if (texture_ == nullptr) {
    throw std::runtime_error{
        std::format("Error creating texture: {}", SDL_GetError())};
  }
  spdlog::trace("SDL Texture created successfully");
}

bool FrameRenderer::renderFrame(const cv::Mat& frame) const noexcept {
  SDL_SetRenderDrawColor(renderer_.get(), 0, 0, 0, 0);
  SDL_RenderClear(renderer_.get());

  cv::Mat paddedFrame;
  cv::copyMakeBorder(frame, paddedFrame, padding_, padding_, padding_, padding_,
                     cv::BORDER_CONSTANT);

  void* pixels;
  int32_t pitch;
  if (!SDL_LockTexture(texture_.get(), nullptr, &pixels, &pitch)) {
    spdlog::error("Error creating texture: {}", SDL_GetError());
    return false;
  }
  std::memcpy(pixels, paddedFrame.data, frame.elemSize() * frame.total());
  SDL_UnlockTexture(texture_.get());

  if (!SDL_RenderTexture(renderer_.get(), texture_.get(), nullptr, nullptr) ||
      !SDL_RenderPresent(renderer_.get())) {
    spdlog::error("Error rendering frame: {}", SDL_GetError());
    return false;
  }
  return true;
}

FrameRenderer::~FrameRenderer() { SDL_QuitSubSystem(SDL_INIT_VIDEO); }
}  // namespace mvplayer
