#include "frame_renderer.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_scancode.h>
#include <spdlog/spdlog.h>

#include <atomic>
#include <format>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "events.hpp"
#include "sdl_manager.hpp"
#include "utils/owned.hpp"

namespace mvplayer {

frame_renderer::frame_renderer(int32_t padding) : padding_{padding} {}

frame_renderer::frame_renderer(frame_renderer&& renderer) noexcept
    : event_listener_{std::move(renderer.event_listener_)},
      window_{renderer.window_.release()},
      renderer_{renderer.renderer_.release()},
      texture_{renderer.texture_.release()},
      width_{renderer.width_},
      height_{renderer.height_},
      padding_{renderer.padding_},
      is_terminated_{renderer.is_terminated_.load(std::memory_order_relaxed)} {}

frame_renderer& frame_renderer::operator=(frame_renderer&& renderer) noexcept {
  event_listener_ = std::move(renderer.event_listener_);
  window_ = sdl_window{renderer.window_.release()};
  renderer_ = sdl_renderer{renderer.renderer_.release()};
  texture_ = sdl_texture{renderer.texture_.release()};
  width_ = renderer.width_;
  height_ = renderer.height_;
  padding_ = renderer.padding_;
  is_terminated_ = renderer.is_terminated_.load(std::memory_order_relaxed);
  return *this;
}

void frame_renderer::operator()(const new_video_loaded_event& event) {
  width_ = event.payload().info.width;
  height_ = event.payload().info.height;

  int32_t padded_width = width_ + (2 * padding_);
  int32_t padded_height = height_ + (2 * padding_);

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
  event_listener_ = std::thread{&frame_renderer::event_listener, this};
}

void frame_renderer::operator()(const new_frame_loaded_event& event) {
  SDL_SetRenderDrawColor(renderer_.get(), 0, 0, 0, 0);
  SDL_RenderClear(renderer_.get());

  cv::Mat padded_frame;
  cv::copyMakeBorder(event.payload().frame, padded_frame, padding_, padding_,
                     padding_, padding_, cv::BORDER_CONSTANT);

  void* pixels{nullptr};
  int32_t pitch{};
  if (!SDL_LockTexture(texture_.get(), nullptr, &pixels, &pitch)) {
    spdlog::error("Error creating texture: {}", SDL_GetError());
    std::ignore = event_handler_t::request_termination();
  }
  std::memcpy(pixels, padded_frame.data,
              event.payload().frame.elemSize() * event.payload().frame.total());
  SDL_UnlockTexture(texture_.get());

  if (!SDL_RenderTexture(renderer_.get(), texture_.get(), nullptr, nullptr) ||
      !SDL_RenderPresent(renderer_.get())) {
    spdlog::error("Error rendering frame: {}", SDL_GetError());
    std::ignore = event_handler_t::request_termination();
  }
}

frame_renderer::~frame_renderer() noexcept {
  if (event_listener_.joinable()) {
    event_listener_.join();
  }
  SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void frame_renderer::event_listener() noexcept {
  SDL_Event sdl_event;
  while (!is_terminated_.load(std::memory_order_relaxed)) {
    while (SDL_PollEvent(&sdl_event)) {
      if (sdl_event.type == SDL_EVENT_QUIT) {
        std::ignore = request_termination();
        return;
      }
    }
  }
}

}  // namespace mvplayer
