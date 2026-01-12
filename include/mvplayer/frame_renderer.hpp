#pragma once

#include <SDL3/SDL.h>

#include <atomic>
#include <cstdint>
#include <opencv2/core.hpp>

#include "events.hpp"
#include "events/envelope.hpp"
#include "events/handler.hpp"
#include "utils/owned.hpp"

namespace mvplayer {
class frame_renderer
    : public multithreaded::events::handlers<events::new_frame_loaded,
                                             events::new_video_loaded> {
 private:
  using new_frame_loaded_event =
      multithreaded::events::envelope<events::new_frame_loaded>;
  using new_video_loaded_event =
      multithreaded::events::envelope<events::new_video_loaded>;

 public:
  frame_renderer(const frame_renderer&) = delete;
  frame_renderer& operator=(const frame_renderer&) = delete;

  frame_renderer(frame_renderer&&) noexcept;
  frame_renderer& operator=(frame_renderer&&) noexcept;

  void on_startup([[maybe_unused]] std::span<char* const> args) noexcept {}

  [[nodiscard]] explicit frame_renderer(int32_t padding);
  void operator()(const new_frame_loaded_event& event) override;
  void operator()(const new_video_loaded_event& event) override;
  [[nodiscard]] bool render_frame(const cv::Mat& frame) const noexcept;
  void start() const noexcept;
  ~frame_renderer() noexcept override;

 private:
  void event_listener() noexcept;

  std::thread event_listener_;
  sdl_window window_{nullptr};
  sdl_renderer renderer_{nullptr};
  sdl_texture texture_{nullptr};
  int32_t width_{}, height_{}, padding_{};
  std::atomic<bool> is_terminated_{false};
};
}  // namespace mvplayer
