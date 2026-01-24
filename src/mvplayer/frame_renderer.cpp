#include "frame_renderer.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_timer.h>
#include <spdlog/spdlog.h>

#include <atomic>
#include <chrono>
#include <format>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <thread>

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
      playback_state_{std::move(renderer.playback_state_)},
      width_{renderer.width_},
      height_{renderer.height_},
      padding_{renderer.padding_},
      is_terminated_{renderer.is_terminated_.load(std::memory_order_acquire)} {}

frame_renderer& frame_renderer::operator=(frame_renderer&& renderer) noexcept {
  event_listener_ = std::move(renderer.event_listener_);
  window_ = sdl_window{renderer.window_.release()};
  renderer_ = sdl_renderer{renderer.renderer_.release()};
  texture_ = sdl_texture{renderer.texture_.release()};
  playback_state_ = std::move(renderer.playback_state_);
  width_ = renderer.width_;
  height_ = renderer.height_;
  padding_ = renderer.padding_;
  is_terminated_ = renderer.is_terminated_.load(std::memory_order_acquire);
  return *this;
}

void frame_renderer::operator()(const new_video_loaded_event& event) {
  width_ = event.payload().info.picture.width;
  height_ = event.payload().info.picture.height;

  playback_state_.first_frame_render_ts = 0;
  playback_state_.timebase = event.payload().info.picture.tbn;

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
  if (event.payload().frame_num != playback_state_.expected_frame_no) {
    spdlog::critical(
        "[frame-renderer] Expected frame no {}, {} found. Lost {} frames",
        playback_state_.expected_frame_no, event.payload().frame_num,
        event.payload().frame_num - playback_state_.expected_frame_no);
  }

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

  auto curr_frame_ts = SDL_GetTicks();
  spdlog::trace("Current frame timestamp: {}. PTS = {}. Timebase = {} / {}",
                curr_frame_ts, event.payload().frame_pts,
                playback_state_.timebase.num, playback_state_.timebase.den);

  if (playback_state_.expected_frame_no == 1) {
    // If no frames are rendered yet, use curr_frame_ts as base timestamp for
    // future renders
    playback_state_.first_frame_render_ts = curr_frame_ts;
  } else {
    auto pts_in_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::seconds(event.payload().frame_pts))
                         .count();
    auto expected_render_time =
        playback_state_.first_frame_render_ts +
        playback_state_.extra_time.load(std::memory_order_acquire) +
        static_cast<uint64_t>(std::ceil(
            static_cast<double>(pts_in_ms * playback_state_.timebase.num) /
            playback_state_.timebase.den));
    auto wait_time = expected_render_time - curr_frame_ts;
    if (wait_time < 0) {
      spdlog::critical("Frame renderer is {}ms behind", -wait_time);
    } else {
      SDL_Delay(wait_time);
      spdlog::trace("Slept for {}ms", wait_time);
    }
  }

  if (!SDL_RenderTexture(renderer_.get(), texture_.get(), nullptr, nullptr) ||
      !SDL_RenderPresent(renderer_.get())) {
    spdlog::error("Error rendering frame: {}", SDL_GetError());
    std::ignore = event_handler_t::request_termination();
  }

  playback_state_.expected_frame_no++;
}

frame_renderer::~frame_renderer() noexcept {
  if (event_listener_.joinable()) {
    event_listener_.join();
  }
}

void frame_renderer::event_listener() noexcept {
  SDL_Event sdl_event;
  int64_t stop_counter = 0;
  while (!is_terminated_.load(std::memory_order_acquire)) {
    while (SDL_PollEvent(&sdl_event)) {
      if (sdl_event.type == SDL_EVENT_QUIT) {
        std::ignore = request_termination();
        return;
      }
      if (sdl_event.type == SDL_EVENT_KEY_DOWN) {
        if (sdl_event.key.scancode == SDL_SCANCODE_SPACE) {
          const auto current_ts =
              std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::high_resolution_clock::now().time_since_epoch())
                  .count();
          const auto paused_status = is_paused_.load(std::memory_order_relaxed);
          if (!paused_status) {
            stop_counter -= current_ts;
          } else {
            stop_counter += current_ts;
            playback_state_.extra_time.fetch_add(stop_counter,
                                                 std::memory_order_acq_rel);
            stop_counter = 0;
          }
          is_paused_.store(!paused_status, std::memory_order_release);
          broadcast(events::playback_toggled{});
        }
      }
    }
  }
}

}  // namespace mvplayer
