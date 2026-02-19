#include "frame_renderer.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_timer.h>

#include <functional>

#include "utils/constants.hpp"
extern "C" {
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixdesc.h>
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}
#include <spdlog/spdlog.h>

#include <atomic>
#include <chrono>
#include <format>
#include <thread>

#include "events.hpp"
#include "sdl_manager.hpp"
#include "utils/owned.hpp"

namespace mvplayer {

frame_renderer::frame_renderer(
    int32_t padding, const std::reference_wrapper<frame_pool>& frame_pool)
    : frame_pool_{frame_pool}, padding_{padding} {}

frame_renderer::frame_renderer(frame_renderer&& renderer) noexcept
    : event_listener_{std::move(renderer.event_listener_)},
      window_{renderer.window_.release()},
      renderer_{renderer.renderer_.release()},
      texture_{renderer.texture_.release()},
      frame_pool_{renderer.frame_pool_},
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
  frame_pool_ = renderer.frame_pool_;
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
      sdl_manager::create_window("video-player", WINDOW_WIDTH, WINDOW_HEIGHT);
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

  SDL_SetRenderLogicalPresentation(
      renderer_.get(), padded_width, padded_height,
      SDL_RendererLogicalPresentation::SDL_LOGICAL_PRESENTATION_LETTERBOX);

  render_roi_ = {.x = static_cast<float>(padding_),
                 .y = static_cast<float>(padding_),
                 .w = static_cast<float>(width_),
                 .h = static_cast<float>(height_)};
  frame_roi_ = {.x = 0, .y = 0, .w = width_, .h = height_};
  spdlog::trace("SDL Renderer created successfully");

  texture_ = sdl_manager::create_texture(
      renderer_.get(), SDL_PixelFormat::SDL_PIXELFORMAT_IYUV,
      SDL_TextureAccess::SDL_TEXTUREACCESS_STREAMING, width_, height_);
  if (texture_ == nullptr) {
    throw std::runtime_error{
        std::format("Error creating texture: {}", SDL_GetError())};
  }

  if (converted_frame_holder_ != nullptr) {
    av_freep(static_cast<void*>(&converted_frame_holder_->data[0]));
    av_frame_free(&converted_frame_holder_);
  }
  if (conversion_context_ != nullptr) {
    sws_free_context(&conversion_context_);
  }

  spdlog::trace("SDL Texture created successfully");
  event_listener_ = std::thread{&frame_renderer::event_listener, this};
}

bool frame_renderer::initialize_converted_frame_holder(
    AVFrame* frame) noexcept {
  converted_frame_holder_ = av_frame_alloc();
  if (converted_frame_holder_ == nullptr) {
    return false;
  }
  if (av_image_alloc(static_cast<uint8_t**>(converted_frame_holder_->data),
                     static_cast<int32_t*>(converted_frame_holder_->linesize),
                     frame->width, frame->height, SUPPORTED_FORMAT,
                     FRAME_ALLOC_ALIGNMENT) < 0) {
    av_frame_free(&converted_frame_holder_);
    return false;
  }
  converted_frame_holder_->width = frame->width;
  converted_frame_holder_->height = frame->height;
  converted_frame_holder_->format = SUPPORTED_FORMAT;

  return true;
}

bool frame_renderer::convert_frame(AVFrame* frame) noexcept {
  if (converted_frame_holder_ == nullptr &&
      !initialize_converted_frame_holder(frame)) {
    return false;
  }

  converted_frame_holder_->pts = frame->pts;

  if (conversion_context_ == nullptr) {
    conversion_context_ = sws_getContext(
        frame->width, frame->height, static_cast<AVPixelFormat>(frame->format),
        converted_frame_holder_->width, converted_frame_holder_->height,
        SUPPORTED_FORMAT,
        SWS_FAST_BILINEAR | SWS_FULL_CHR_H_INT | SWS_ACCURATE_RND,  // NOLINT
        nullptr, nullptr, nullptr);
  }

  sws_scale(conversion_context_, static_cast<uint8_t**>(frame->data),
            static_cast<int32_t*>(frame->linesize), 0, frame->height,
            static_cast<uint8_t**>(converted_frame_holder_->data),
            static_cast<int32_t*>(converted_frame_holder_->linesize));

  return true;
}

void frame_renderer::operator()(const new_frame_loaded_event& event) {
  const auto& payload = event.payload();
  if (payload.frame_num != playback_state_.expected_frame_no) [[unlikely]] {
    spdlog::critical(
        "[frame-renderer] Expected frame no {}, {} found. Lost {} frames",
        playback_state_.expected_frame_no, event.payload().frame_num,
        event.payload().frame_num - playback_state_.expected_frame_no);
    playback_state_.expected_frame_no = event.payload().frame_num;
  }

  SDL_SetRenderDrawColor(renderer_.get(), 0, 0, 0, 0);
  SDL_RenderClear(renderer_.get());

  auto* frame = payload.frame;
  bool swap_frame = false;
  if (frame->format != SUPPORTED_FORMAT) {
    if (!convert_frame(frame)) {
      spdlog::error("Error converting frame to {}",
                    av_get_pix_fmt_name(SUPPORTED_FORMAT));
      std::ignore = request_termination();
    }
    std::swap(frame, converted_frame_holder_);
    swap_frame = true;
  }
  if (!SDL_UpdateYUVTexture(texture_.get(), &frame_roi_, frame->data[0],
                            frame->linesize[0], frame->data[1],
                            frame->linesize[1], frame->data[2],
                            frame->linesize[2])) {
    spdlog::error("Error updating texture: {}", SDL_GetError());
    std::ignore = event_handler_t::request_termination();
  }
  if (swap_frame) {
    std::swap(frame, converted_frame_holder_);
  }

  auto curr_frame_ts = SDL_GetTicks();
  spdlog::trace("Current frame timestamp: {}. PTS = {}. Timebase = {} / {}",
                curr_frame_ts, payload.frame_pts, playback_state_.timebase.num,
                playback_state_.timebase.den);

  if (playback_state_.expected_frame_no == 1 || payload.reset_frame_sequence) {
    // If no frames are rendered yet, use curr_frame_ts as base timestamp for
    // future renders
    playback_state_.first_frame_render_ts = curr_frame_ts;
    playback_state_.first_frame_pts = payload.frame_pts;
    playback_state_.extra_time.store(0, std::memory_order_release);
  } else {
    auto pts_in_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::seconds(event.payload().frame_pts -
                                              playback_state_.first_frame_pts))
                         .count();
    auto expected_render_time =
        playback_state_.first_frame_render_ts +
        playback_state_.extra_time.load(std::memory_order_acquire) +
        av_rescale_q(pts_in_ms, playback_state_.timebase,
                     AVRational{.num = 1, .den = 1});
    if (expected_render_time < curr_frame_ts) {
      spdlog::critical("Frame renderer is {}ms behind for frame no {}",
                       curr_frame_ts - expected_render_time,
                       event.payload().frame_num);
      playback_state_.extra_time.fetch_add(curr_frame_ts -
                                           expected_render_time);
      playback_state_.expected_frame_no++;
      frame_pool_.get().release_frame(frame);
      return;
    }
    std::chrono::milliseconds wait_time{expected_render_time - curr_frame_ts};
    SDL_DelayPrecise(
        std::chrono::duration_cast<std::chrono::nanoseconds>(wait_time)
            .count());
    spdlog::trace("Slept for {}ms", wait_time.count());
  }

  if (!SDL_RenderTexture(renderer_.get(), texture_.get(), nullptr,
                         &render_roi_) ||
      !SDL_RenderPresent(renderer_.get())) {
    spdlog::error("Error rendering frame: {}", SDL_GetError());
    std::ignore = event_handler_t::request_termination();
  }

  playback_state_.expected_frame_no++;
  frame_pool_.get().release_frame(frame);
}

frame_renderer::~frame_renderer() noexcept {
  if (converted_frame_holder_ != nullptr) {
    av_freep(static_cast<void*>(&converted_frame_holder_->data[0]));
    av_frame_free(&converted_frame_holder_);
  }
  if (conversion_context_ != nullptr) {
    sws_freeContext(conversion_context_);
  }
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
        switch (sdl_event.key.scancode) {
          case SDL_SCANCODE_SPACE: {
            const auto current_ts =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::high_resolution_clock::now()
                        .time_since_epoch())
                    .count();
            const auto paused_status =
                is_paused_.load(std::memory_order_relaxed);
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
            break;
          }
          case SDL_SCANCODE_LEFT: {
            broadcast(
                events::seek_request{.direction = seek_direction::backward});
            break;
          }
          case SDL_SCANCODE_RIGHT: {
            broadcast(
                events::seek_request{.direction = seek_direction::forward});
            break;
          }
          default: {
            break;
          }
        }
      }
    }
    std::this_thread::sleep_for(constants::POLL_INTERVAL);
  }
}

}  // namespace mvplayer
