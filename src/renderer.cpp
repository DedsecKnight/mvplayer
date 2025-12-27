#include "renderer.hpp"

#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>

#include <format>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

namespace mvplayer {
Renderer::Renderer(int32_t width, int32_t height, int32_t padding)
    : width_{width}, height_{height}, padding_{padding} {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    throw std::runtime_error{
        std::format("Error initializing SDL: {}", SDL_GetError())};
  }
  int32_t paddedWidth = width_ + 2 * padding_;
  int32_t paddedHeight = height_ + 2 * padding_;

  window_ = SDL_CreateWindow("video-player", paddedWidth, paddedHeight,
                             SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS);
  if (window_ == nullptr) {
    throw std::runtime_error{
        std::format("Error creating window: {}", SDL_GetError())};
  }
  spdlog::trace("SDL Window created successfully");

  renderer_ = SDL_CreateRenderer(window_.get(), nullptr);
  if (renderer_ == nullptr) {
    throw std::runtime_error{
        std::format("Error creating renderer: {}", SDL_GetError())};
  }
  spdlog::trace("SDL Renderer created successfully");

  texture_ =
      SDL_CreateTexture(renderer_.get(), SDL_PixelFormat::SDL_PIXELFORMAT_RGB24,
                        SDL_TextureAccess::SDL_TEXTUREACCESS_STREAMING,
                        paddedWidth, paddedHeight);
  if (texture_ == nullptr) {
    throw std::runtime_error{
        std::format("Error creating texture: {}", SDL_GetError())};
  }
  spdlog::trace("SDL Texture created successfully");
}

bool Renderer::renderFrame(const cv::Mat& frame) const noexcept {
  SDL_SetRenderDrawColor(renderer_.get(), 0, 0, 0, 0);
  SDL_RenderClear(renderer_.get());

  cv::Mat paddedFrame;
  cv::copyMakeBorder(frame, paddedFrame, padding_, padding_, padding_, padding_,
                     cv::BORDER_CONSTANT);

  void* pixels;
  int pitch;
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

Renderer::~Renderer() {
  // if (texture_ != nullptr) {
  //   SDL_DestroyTexture(texture_);
  // }
  // if (renderer_ != nullptr) {
  //   SDL_DestroyRenderer(renderer_);
  // }
  // if (window_ != nullptr) {
  //   SDL_DestroyWindow(window_);
  // }
  SDL_Quit();
}
}  // namespace mvplayer
