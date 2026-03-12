#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>

#include "processor/pre_event_handler.hpp"

extern "C" {
#include <libswscale/swscale.h>
}

#include <array>
#include <atomic>
#include <cstdint>
#include <span>

#include "events.hpp"
#include "events/envelope.hpp"
#include "events/handler.hpp"
#include "frame_pool.hpp"
#include "processor/interruptible.hpp"
#include "renderer/base.hpp"
#include "utils/constants.hpp"
#include "utils/owned.hpp"

namespace mvplayer {

class frame_renderer
    : public multithreaded::events::handlers<events::new_frame_loaded,
                                             events::new_video_loaded>,
      public multithreaded::processor::interruptible_processor,
      public multithreaded::processor::pre_event_handler {
 private:
  using new_frame_loaded_event =
      multithreaded::events::envelope<events::new_frame_loaded>;
  using new_video_loaded_event =
      multithreaded::events::envelope<events::new_video_loaded>;
  static constexpr AVPixelFormat SUPPORTED_FORMAT =
      AVPixelFormat::AV_PIX_FMT_RGB24;
  static constexpr int32_t WINDOW_HEIGHT = 1440;
  static constexpr int32_t WINDOW_WIDTH = 2560;
  static constexpr int32_t FRAME_ALLOC_ALIGNMENT = 32;

  struct video_playback_state {
    video_playback_state() = default;
    video_playback_state(const video_playback_state& state)
        : extra_time{state.extra_time.load(std::memory_order_acquire)},
          timebase{state.timebase},
          first_frame_render_ts{state.first_frame_render_ts},
          first_frame_pts{state.first_frame_pts},
          expected_frame_no{state.expected_frame_no} {}

    video_playback_state& operator=(
        const video_playback_state& state) noexcept {
      if (this == &state) {
        return *this;
      }
      extra_time = state.extra_time.load(std::memory_order_acquire);
      timebase = state.timebase;
      first_frame_render_ts = state.first_frame_render_ts;
      first_frame_pts = state.first_frame_pts;
      expected_frame_no = state.expected_frame_no;
      return *this;
    }

    video_playback_state(video_playback_state&& state) noexcept
        : extra_time{state.extra_time.load(std::memory_order_acquire)},
          timebase{state.timebase},
          first_frame_render_ts{state.first_frame_render_ts},
          first_frame_pts{state.first_frame_pts},
          expected_frame_no{state.expected_frame_no} {}

    video_playback_state& operator=(video_playback_state&& state) noexcept {
      extra_time = state.extra_time.load(std::memory_order_acquire);
      timebase = state.timebase;
      first_frame_render_ts = state.first_frame_render_ts;
      first_frame_pts = state.first_frame_pts;
      expected_frame_no = state.expected_frame_no;
      return *this;
    }

    ~video_playback_state() = default;

    alignas(constants::CACHE_LINE_SIZE) std::atomic<uint64_t> extra_time;
    std::array<uint8_t, constants::CACHE_LINE_SIZE - sizeof(extra_time)>
        padding{};

    AVRational timebase{};
    uint64_t first_frame_render_ts{}, first_frame_pts{};
    int64_t expected_frame_no{1};
  };

 public:
  frame_renderer(const frame_renderer&) = delete;
  frame_renderer& operator=(const frame_renderer&) = delete;

  frame_renderer(frame_renderer&&) noexcept;
  frame_renderer& operator=(frame_renderer&&) noexcept;

  void on_startup([[maybe_unused]] std::span<char* const> args) noexcept {}
  [[nodiscard]] bool halt_requested() const noexcept override {
    return is_paused_;
  }

  void pre_event() noexcept override;

  [[nodiscard]] explicit frame_renderer(
      int32_t padding, const std::reference_wrapper<frame_pool>& frame_pool);
  void operator()(const new_frame_loaded_event& event) override;
  void operator()(const new_video_loaded_event& event) override;
  ~frame_renderer() noexcept override;

 private:
  bool convert_frame(AVFrame* frame) noexcept;
  bool initialize_converted_frame_holder(AVFrame* frame) noexcept;
  void initialize_viewport(int32_t width, int32_t height) const noexcept;

  sdl_window window_{nullptr};
  sdl_gl_context context_{nullptr};
  std::unique_ptr<renderer::base> renderer_{nullptr};
  std::reference_wrapper<frame_pool> frame_pool_;
  video_playback_state playback_state_;
  SwsContext* conversion_context_{nullptr};
  AVFrame* converted_frame_holder_{nullptr};
  int64_t stop_counter_{};
  int32_t width_{}, height_{}, padding_{};
  bool is_paused_{false};
};
}  // namespace mvplayer
