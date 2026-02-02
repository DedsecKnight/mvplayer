#include "audio_renderer.hpp"

#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_timer.h>

#include "sdl_manager.hpp"
#include "utils/conversion.hpp"
#include "utils/owned.hpp"

namespace mvplayer {
audio_renderer::audio_renderer(audio_renderer&& renderer) noexcept
    : playback_state_{renderer.playback_state_},
      audio_stream_{renderer.audio_stream_.release()},
      is_paused_{renderer.is_paused_} {}

audio_renderer& audio_renderer::operator=(audio_renderer&& renderer) noexcept {
  playback_state_ = renderer.playback_state_;
  audio_stream_ = sdl_audio_stream{renderer.audio_stream_.release()};
  is_paused_ = renderer.is_paused_;
  return *this;
}

void audio_renderer::operator()(const new_audio_samples_loaded_event& event) {
  if (event.payload().frame_num != playback_state_.expected_frame_no) {
    spdlog::critical(
        "[audio-renderer] Expected frame no {}, {} found. Lost {} frames",
        playback_state_.expected_frame_no, event.payload().frame_num,
        event.payload().frame_num - playback_state_.expected_frame_no);
    playback_state_.expected_frame_no = event.payload().frame_num;
  }

  if (event.payload().reset_frame_sequence) {
    SDL_ClearAudioStream(audio_stream_.get());
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
  if (!event.payload().info.audio.has_audio_stream) {
    return;
  }
  const auto& audio_info = event.payload().info.audio;
  const auto sdl_format = utils::to_sdl_format(audio_info.sample_format);
  audio_stream_ = sdl_manager::create_audio_stream(
      sdl_format, audio_info.num_channels, audio_info.frequency);
  if (audio_stream_ == nullptr) {
    throw std::runtime_error{
        std::format("Error creating audio stream: {}", SDL_GetError())};
  }

  spdlog::trace("SDL Audio Stream created successfully");
  SDL_ResumeAudioStreamDevice(audio_stream_.get());
}

void audio_renderer::operator()(
    [[maybe_unused]] const playback_toggled_event& event) {
  if (!is_paused_) {
    SDL_FlushAudioStream(audio_stream_.get());
    SDL_PauseAudioStreamDevice(audio_stream_.get());
  } else {
    SDL_ResumeAudioStreamDevice(audio_stream_.get());
  }
  is_paused_ = !is_paused_;
}

}  // namespace mvplayer
