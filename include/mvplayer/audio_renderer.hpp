#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>

#include "error.hpp"
#include "frame_pool.hpp"

extern "C" {
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

#include <cstdint>
#include <span>

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
    int64_t expected_frame_no{1};
  };

  static constexpr size_t DEFAULT_AUDIO_BUFFER_SIZE = 192'000;

 public:
  explicit audio_renderer(const std::reference_wrapper<frame_pool>& frame_pool)
      : frame_pool_{frame_pool} {}

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

  ~audio_renderer() noexcept override;

 private:
  [[nodiscard]] std::expected<std::span<uint8_t>, error> generate_audio_buffer(
      AVFrame* frame, int32_t num_channels) noexcept;
  [[nodiscard]] std::expected<std::span<uint8_t>, error>
  generate_packed_planar_sample(AVFrame* frame, int32_t num_channels) noexcept;
  [[nodiscard]] std::expected<std::span<uint8_t>, error>
  generate_non_planar_sample(AVFrame* frame, int32_t num_channels) noexcept;

  [[nodiscard]] std::expected<void, error> initialize_swr_context(
      AVSampleFormat input_fmt, AVSampleFormat output_fmt) noexcept;

  std::vector<uint8_t*> av_sample_buffer_{nullptr};
  AVChannelLayout* channel_layout_ptr_{nullptr};
  audio_playback_state playback_state_{};
  sdl_audio_stream audio_stream_{nullptr};
  SwrContext* resample_context_{nullptr};
  std::reference_wrapper<frame_pool> frame_pool_;
  // Stores the number of bytes that actually contains audio data
  size_t audio_buffer_size_{};
  int32_t audio_sample_rate_{}, max_num_samples_{};
  bool is_paused_{false};
};
}  // namespace mvplayer
