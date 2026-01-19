#include "audio_renderer.hpp"

#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_timer.h>

#include <atomic>

#include "sdl_manager.hpp"
#include "utils/conversion.hpp"
#include "utils/owned.hpp"

namespace mvplayer {
// TODO: implement move for playback_state_
audio_renderer::audio_renderer(audio_renderer&& renderer) noexcept
    : audio_stream_{renderer.audio_stream_.release()} {}

audio_renderer& audio_renderer::operator=(audio_renderer&& renderer) noexcept {
  audio_stream_ = sdl_audio_stream{renderer.audio_stream_.release()};
  return *this;
}

void audio_renderer::operator()(const new_audio_samples_loaded_event& event) {
  if (event.payload().frame_num != playback_state_.expected_frame_no) {
    spdlog::critical(
        "Expected frame no {}, {} found. Lost {} frames",
        playback_state_.expected_frame_no, event.payload().frame_num,
        event.payload().frame_num - playback_state_.expected_frame_no);
  }

  auto curr_frame_ts = SDL_GetTicks();
  spdlog::trace("Current frame timestamp: {}. PTS = {}. Timebase = {} / {}",
                curr_frame_ts, event.payload().frame_pts,
                playback_state_.timebase.num, playback_state_.timebase.den);

  if (playback_state_.expected_frame_no == 1) {
    playback_state_.first_frame_render_ts = curr_frame_ts;
    SDL_ResumeAudioStreamDevice(audio_stream_.get());
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
  const auto& payload = event.payload();

  if (!SDL_PutAudioStreamData(audio_stream_.get(), payload.samples.data(),
                              static_cast<int32_t>(payload.samples.size()))) {
    spdlog::error("Error enqueueing audio sample: {}", SDL_GetError());
    std::ignore = request_termination();
  }
  if (!SDL_FlushAudioStream(audio_stream_.get())) {
    spdlog::error("Error flushing audio stream: {}", SDL_GetError());
    std::ignore = request_termination();
  }
  playback_state_.expected_frame_no++;
}

void audio_renderer::operator()(const new_video_loaded_event& event) {
  const auto& audio_info = event.payload().info.audio;
  const auto sdl_format = utils::to_sdl_format(audio_info.sample_format);
  audio_stream_ = sdl_manager::create_audio_stream(
      sdl_format, audio_info.num_channels, audio_info.frequency);
  if (audio_stream_ == nullptr) {
    throw std::runtime_error{
        std::format("Error creating audio stream: {}", SDL_GetError())};
  }

  playback_state_.first_frame_render_ts = 0;
  playback_state_.timebase = event.payload().info.audio.tbn;

  spdlog::trace("SDL Audio Stream created successfully");
}

void audio_renderer::operator()(
    [[maybe_unused]] const playback_toggled_event& event) {
  if (!is_paused_) {
    SDL_PauseAudioStreamDevice(audio_stream_.get());
  } else {
    SDL_ResumeAudioStreamDevice(audio_stream_.get());
  }
  is_paused_ = !is_paused_;
}

}  // namespace mvplayer
