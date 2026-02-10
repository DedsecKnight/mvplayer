#include "audio_renderer.hpp"

#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_timer.h>
#include <spdlog/spdlog.h>

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
}

#include "sdl_manager.hpp"
#include "utils/conversion.hpp"
#include "utils/owned.hpp"

namespace mvplayer {
audio_renderer::audio_renderer(audio_renderer&& renderer) noexcept
    : playback_state_{renderer.playback_state_},
      audio_stream_{renderer.audio_stream_.release()},
      frame_pool_{renderer.frame_pool_},
      is_paused_{renderer.is_paused_} {}

audio_renderer& audio_renderer::operator=(audio_renderer&& renderer) noexcept {
  playback_state_ = renderer.playback_state_;
  audio_stream_ = sdl_audio_stream{renderer.audio_stream_.release()};
  is_paused_ = renderer.is_paused_;
  return *this;
}

bool audio_renderer::initialize_swr_context(
    AVSampleFormat input_fmt, AVSampleFormat output_fmt) noexcept {
  resample_context_ = swr_alloc();

  if (resample_context_ == nullptr) {
    spdlog::error("Error creating SwrContext");
    return false;
  }

  av_opt_set_chlayout(resample_context_, "in_chlayout", channel_layout_ptr_, 0);
  av_opt_set_chlayout(resample_context_, "out_chlayout", channel_layout_ptr_,
                      0);

  av_opt_set_int(resample_context_, "in_sample_rate", audio_sample_rate_, 0);
  av_opt_set_int(resample_context_, "out_sample_rate", audio_sample_rate_, 0);

  av_opt_set_sample_fmt(resample_context_, "in_sample_fmt", input_fmt, 0);
  av_opt_set_sample_fmt(resample_context_, "out_sample_fmt", output_fmt, 0);

  if (swr_init(resample_context_) < 0) {
    spdlog::error("Error initializing SwrContext");
    swr_free(&resample_context_);
    return false;
  }

  return true;
}

std::vector<uint8_t> audio_renderer::generate_non_planar_sample(
    AVFrame* frame, int32_t num_channels) noexcept {
  std::vector<uint8_t> audio_buffer;
  const auto input_format = static_cast<AVSampleFormat>(frame->format);
  const auto num_samples = frame->nb_samples;
  const auto sample_size =
      static_cast<size_t>(av_get_bytes_per_sample(input_format));
  const auto buffer_size = sample_size * num_samples * num_channels;
  audio_buffer.resize(buffer_size);
  std::memcpy(audio_buffer.data(), frame->data[0], audio_buffer.size());
  return audio_buffer;
}

std::vector<uint8_t> audio_renderer::generate_packed_planar_sample(
    AVFrame* frame, int32_t num_channels) noexcept {
  const auto input_fmt = static_cast<AVSampleFormat>(frame->format);
  const auto output_fmt = av_get_packed_sample_fmt(input_fmt);

  if (resample_context_ == nullptr &&
      !initialize_swr_context(input_fmt, output_fmt)) {
    spdlog::error("Error initializing swresample context");
    return {};
  }

  std::vector<uint8_t*> out_data{nullptr};
  [[maybe_unused]] int32_t out_linesize{};

  const auto out_sample_size = av_get_bytes_per_sample(output_fmt);

  const auto ret =
      av_samples_alloc(out_data.data(), &out_linesize, num_channels,
                       frame->nb_samples, output_fmt, 0);

  if (ret < 0) {
    spdlog::error("Error allocating samples");
    return {};
  }

  int32_t num_samples_written = swr_convert(
      resample_context_, out_data.data(), frame->nb_samples,
      static_cast<const uint8_t* const*>(frame->data), frame->nb_samples);

  if (num_samples_written < 0) {
    spdlog::error("Error writing samples");
    return {};
  }

  if (num_samples_written != frame->nb_samples) {
    spdlog::error("Expected to write {} samples. Only {} samples are written",
                  frame->nb_samples, num_samples_written);
    return {};
  }

  size_t audio_buffer_size =
      static_cast<size_t>(out_sample_size) * num_channels * num_samples_written;
  std::vector<uint8_t> audio_buffer(audio_buffer_size);
  std::memcpy(audio_buffer.data(), out_data[0], audio_buffer_size);
  av_freep(static_cast<void*>(out_data.data()));

  return audio_buffer;
}

std::vector<uint8_t> audio_renderer::generate_audio_buffer(
    AVFrame* frame, int32_t num_channels) noexcept {
  const auto audio_format = static_cast<AVSampleFormat>(frame->format);
  if (av_sample_fmt_is_planar(audio_format) == 1) {
    return generate_packed_planar_sample(frame, num_channels);
  }
  return generate_non_planar_sample(frame, num_channels);
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

  auto* frame = event.payload().frame;
  const auto audio_buffer =
      generate_audio_buffer(frame, event.payload().num_channels);

  if (!SDL_PutAudioStreamData(audio_stream_.get(), audio_buffer.data(),
                              static_cast<int32_t>(audio_buffer.size()))) {
    spdlog::error("Error enqueueing audio sample: {}", SDL_GetError());
    std::ignore = request_termination();
  }
  if (!SDL_FlushAudioStream(audio_stream_.get())) {
    spdlog::error("Error flushing audio stream: {}", SDL_GetError());
    std::ignore = request_termination();
  }
  playback_state_.expected_frame_no++;
  frame_pool_.get().release_frame(frame);
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

  channel_layout_ptr_ = event.payload().info.audio.channel_layout;
  audio_sample_rate_ = event.payload().info.audio.frequency;
  if (resample_context_ != nullptr) {
    swr_free(&resample_context_);
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

audio_renderer::~audio_renderer() noexcept {
  if (resample_context_ != nullptr) {
    swr_free(&resample_context_);
  }
}

}  // namespace mvplayer
