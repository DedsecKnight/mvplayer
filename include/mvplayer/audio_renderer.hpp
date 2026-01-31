#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>

#include <cstdint>
#include <opencv2/core.hpp>

#include "events.hpp"
#include "events/envelope.hpp"
#include "events/handler.hpp"
#include "processor/termination_handler.hpp"
#include "utils/owned.hpp"

namespace mvplayer {

class audio_renderer
    : public multithreaded::events::handlers<events::new_audio_samples_loaded,
                                             events::new_video_loaded,
                                             events::playback_toggled>,
      public multithreaded::processor::termination_handler {
 private:
  using new_audio_samples_loaded_event =
      multithreaded::events::envelope<events::new_audio_samples_loaded>;
  using new_video_loaded_event =
      multithreaded::events::envelope<events::new_video_loaded>;
  using playback_toggled_event =
      multithreaded::events::envelope<events::playback_toggled>;

  struct audio_playback_state {
    AVRational timebase{};
    uint64_t pause_toggled_ts{}, extra_time{}, first_frame_render_ts{},
        first_frame_pts{};
    int64_t expected_frame_no{1};
  };

 public:
  audio_renderer() = default;

  audio_renderer(const audio_renderer&) = delete;
  audio_renderer& operator=(const audio_renderer&) = delete;

  audio_renderer(audio_renderer&&) noexcept;
  audio_renderer& operator=(audio_renderer&&) noexcept;

  void on_startup([[maybe_unused]] std::span<char* const> args) noexcept {}
  void handle_termination_signal() noexcept override {
    SDL_FlushAudioStream(audio_stream_.get());
    SDL_PauseAudioStreamDevice(audio_stream_.get());
  }

  void operator()(const new_audio_samples_loaded_event& event) override;
  void operator()(const new_video_loaded_event& event) override;
  void operator()(const playback_toggled_event& event) override;

  ~audio_renderer() noexcept override = default;

 private:
  audio_playback_state playback_state_{};
  sdl_audio_stream audio_stream_{nullptr};
  bool is_paused_{false};
};
}  // namespace mvplayer
