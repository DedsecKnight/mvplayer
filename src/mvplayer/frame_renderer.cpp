// clang-format off
#include <glad/gl.h>
// clang-format on
#include "frame_renderer.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>

#include <functional>

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

#include "events.hpp"
#include "renderer/factory.hpp"
#include "sdl_manager.hpp"
#include "utils/owned.hpp"

namespace mvplayer {

frame_renderer::frame_renderer(
    int32_t padding, const std::reference_wrapper<frame_pool>& frame_pool)
    : frame_pool_{frame_pool}, padding_{padding} {}

frame_renderer::frame_renderer(frame_renderer&& renderer) noexcept
    : window_{renderer.window_.release()},
      frame_pool_{renderer.frame_pool_},
      playback_state_{std::move(renderer.playback_state_)},
      width_{renderer.width_},
      height_{renderer.height_},
      padding_{renderer.padding_} {}

frame_renderer& frame_renderer::operator=(frame_renderer&& renderer) noexcept {
  window_ = sdl_window{renderer.window_.release()};
  frame_pool_ = renderer.frame_pool_;
  playback_state_ = std::move(renderer.playback_state_);
  width_ = renderer.width_;
  height_ = renderer.height_;
  padding_ = renderer.padding_;
  return *this;
}

void frame_renderer::operator()(const new_video_loaded_event& event) {
  width_ = event.payload().info.picture.width;
  height_ = event.payload().info.picture.height;

  playback_state_.first_frame_render_ts = 0;
  playback_state_.timebase = event.payload().info.picture.tbn;

  if (window_ != nullptr) {
    context_.reset(nullptr);
  }

  window_ =
      sdl_manager::create_window("video-player", WINDOW_WIDTH, WINDOW_HEIGHT);
  if (window_ == nullptr) {
    throw std::runtime_error{
        std::format("Error creating window: {}", SDL_GetError())};
  }
  spdlog::trace("SDL Window created successfully");

  context_ = sdl_manager::create_gl_context(window_.get());
  if (context_ == nullptr) {
    spdlog::error("Error creating OpenGL context: {}", SDL_GetError());
    std::ignore = request_termination();
    return;
  }

  const auto gl_version =
      gladLoadGL(static_cast<GLADloadfunc>(SDL_GL_GetProcAddress));
  spdlog::info("Initialized OpenGL with version {}.{}",
               GLAD_VERSION_MAJOR(gl_version), GLAD_VERSION_MINOR(gl_version));

  initialize_viewport(width_, height_);

  if (converted_frame_holder_ != nullptr) {
    av_freep(static_cast<void*>(&converted_frame_holder_->data[0]));
    av_frame_free(&converted_frame_holder_);
  }
  if (conversion_context_ != nullptr) {
    sws_free_context(&conversion_context_);
  }

  spdlog::trace("SDL Texture created successfully");
}

void frame_renderer::initialize_viewport(int32_t width,
                                         int32_t height) const noexcept {
  int32_t screen_width{};
  int32_t screen_height{};
  SDL_GetWindowSize(window_.get(), &screen_width, &screen_height);
  float aspect_ratio = static_cast<float>(width) / static_cast<float>(height);

  int32_t scaled_width = screen_width;
  auto scaled_height = static_cast<int32_t>(lround(
      (static_cast<float>(scaled_width) / aspect_ratio) + 0.5F));  // NOLINT
  if (scaled_height > screen_height) {
    scaled_height = screen_height;
    scaled_width = static_cast<int32_t>(lround(
        (static_cast<float>(scaled_height) * aspect_ratio) + 0.5F));  // NOLINT
  }

  glViewport((screen_width / 2) - (scaled_width / 2),
             (screen_height / 2) - (scaled_height / 2), scaled_width,
             scaled_height);
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

  if (renderer_ == nullptr) {
    renderer_ = renderer::factory::create_renderer_pipeline(
        static_cast<AVPixelFormat>(SUPPORTED_FORMAT));
  }

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

  if (!renderer_->render_frame(frame)) {
    spdlog::error("error rendering frame");
    std::ignore = event_handler_t::request_termination();
    return;
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

  SDL_GL_SwapWindow(window_.get());

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
}

void frame_renderer::pre_event() noexcept {
  SDL_Event sdl_event;
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
                  std::chrono::high_resolution_clock::now().time_since_epoch())
                  .count();
          if (!is_paused_) {
            stop_counter_ -= current_ts;
          } else {
            stop_counter_ += current_ts;
            playback_state_.extra_time.fetch_add(stop_counter_,
                                                 std::memory_order_acq_rel);
            stop_counter_ = 0;
          }
          is_paused_ = !is_paused_;
          broadcast(events::playback_toggled{});
          break;
        }
        case SDL_SCANCODE_LEFT: {
          broadcast(
              events::seek_request{.direction = seek_direction::backward});
          break;
        }
        case SDL_SCANCODE_RIGHT: {
          broadcast(events::seek_request{.direction = seek_direction::forward});
          break;
        }
        default: {
          break;
        }
      }
    }
  }
}

}  // namespace mvplayer
