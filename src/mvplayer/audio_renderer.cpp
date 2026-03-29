#include "audio_renderer.hpp"

#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_timer.h>
#include <spdlog/spdlog.h>

#include "error.hpp"

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

[[nodiscard]] std::expected<void, error> audio_renderer::initialize_swr_context(
    AVSampleFormat input_fmt, AVSampleFormat output_fmt) noexcept {
  resample_context_ = swr_alloc();

  if (resample_context_ == nullptr) {
    return std::unexpected(
        allocation_error{.context = "initialize_swr_context"});
  }

  av_opt_set_chlayout(resample_context_, "in_chlayout", channel_layout_ptr_, 0);
  av_opt_set_chlayout(resample_context_, "out_chlayout", channel_layout_ptr_,
                      0);

  av_opt_set_int(resample_context_, "in_sample_rate", audio_sample_rate_, 0);
  av_opt_set_int(resample_context_, "out_sample_rate", audio_sample_rate_, 0);

  av_opt_set_sample_fmt(resample_context_, "in_sample_fmt", input_fmt, 0);
  av_opt_set_sample_fmt(resample_context_, "out_sample_fmt", output_fmt, 0);

  if (auto ret = swr_init(resample_context_); ret < 0) {
    swr_free(&resample_context_);
    return std::unexpected(av_error{.code = ret, .context = "swr_init"});
  }

  return {};
}

[[nodiscard]] std::expected<std::span<uint8_t>, error>
audio_renderer::generate_non_planar_sample(AVFrame* frame,
                                           int32_t num_channels) noexcept {
  const auto input_format = static_cast<AVSampleFormat>(frame->format);
  const auto num_samples = frame->nb_samples;
  const auto sample_size =
      static_cast<size_t>(av_get_bytes_per_sample(input_format));
  audio_buffer_size_ = sample_size * num_samples * num_channels;
  return std::span<uint8_t>{frame->data[0], audio_buffer_size_};
}

[[nodiscard]] std::expected<std::span<uint8_t>, error>
audio_renderer::generate_packed_planar_sample(AVFrame* frame,
                                              int32_t num_channels) noexcept {
  const auto input_fmt = static_cast<AVSampleFormat>(frame->format);
  const auto output_fmt = av_get_packed_sample_fmt(input_fmt);

  if (resample_context_ == nullptr) {
    auto swr_ctx_init_ret = initialize_swr_context(input_fmt, output_fmt);
    if (!swr_ctx_init_ret.has_value()) {
      return std::unexpected(swr_ctx_init_ret.error());
    }
  }

  const auto out_sample_size = av_get_bytes_per_sample(output_fmt);
  audio_buffer_size_ =
      static_cast<size_t>(out_sample_size) * num_channels * frame->nb_samples;

  if (max_num_samples_ < frame->nb_samples) {
    av_freep(static_cast<void*>(av_sample_buffer_.data()));
  }
  if (av_sample_buffer_[0] == nullptr) {
    const auto ret =
        av_samples_alloc(av_sample_buffer_.data(), nullptr, num_channels,
                         frame->nb_samples, output_fmt, 0);

    if (ret < 0) {
      return std::unexpected(
          av_error{.code = ret, .context = "av_samples_alloc"});
    }

    max_num_samples_ = frame->nb_samples;
  }

  int32_t num_samples_written = swr_convert(
      resample_context_, av_sample_buffer_.data(), frame->nb_samples,
      static_cast<const uint8_t* const*>(frame->data), frame->nb_samples);

  if (num_samples_written < 0) {
    return std::unexpected(
        av_error{.code = num_samples_written, .context = "swr_convert"});
  }

  if (num_samples_written != frame->nb_samples) {
    spdlog::error("Expected to write {} samples. Only {} samples are written",
                  frame->nb_samples, num_samples_written);
    return std::unexpected(
        mismatch_sample_written_error{.context = "swr_convert",
                                      .expected = frame->nb_samples,
                                      .actual = num_samples_written});
  }

  return std::span<uint8_t>{av_sample_buffer_[0], audio_buffer_size_};
}

[[nodiscard]] std::expected<std::span<uint8_t>, error>
audio_renderer::generate_audio_buffer(AVFrame* frame,
                                      int32_t num_channels) noexcept {
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
  const auto load_audio_result =
      generate_audio_buffer(frame, event.payload().num_channels)
          .and_then([this](std::span<uint8_t> audio_buffer)
                        -> std::expected<void, error> {
            if (!SDL_PutAudioStreamData(
                    audio_stream_.get(), audio_buffer.data(),
                    static_cast<int32_t>(audio_buffer_size_))) {
              return std::unexpected(audio_processing_error{
                  .context = "SDL_PutAudioStreamData", .msg = SDL_GetError()});
            }
            if (!SDL_FlushAudioStream(audio_stream_.get())) {
              return std::unexpected(audio_processing_error{
                  .context = "SDL_FlushAudioStream", .msg = SDL_GetError()});
            }
            return {};
          });
  if (!load_audio_result.has_value()) {
    spdlog::error("Error loading audio sample: {}. Requesting termination...",
                  load_audio_result.error());
    frame_pool_.get().release_frame(frame);
    std::ignore = request_termination();
    return;
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
  if (av_sample_buffer_[0] != nullptr) {
    av_freep(static_cast<void*>(av_sample_buffer_.data()));
  }
}

}  // namespace mvplayer
