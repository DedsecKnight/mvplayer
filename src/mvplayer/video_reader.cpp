#include "video_reader.hpp"

#include <atomic>
#include <filesystem>

#include "CLI/CLI.hpp"
#include "events.hpp"
#include "info.hpp"
#include "media_context.hpp"
#include "utils/conversion.hpp"

extern "C" {
#include <libavcodec/codec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/pixfmt.h>
}

#include <spdlog/spdlog.h>

namespace mvplayer {
void video_reader::on_startup(std::span<char* const> args) noexcept {
  CLI::App app;
  std::string input_filename;
  app.add_option("-i", input_filename, "Input video file name")->required();
  try {
    app.parse(static_cast<int32_t>(args.size()), args.data());
  } catch (const CLI::ParseError& e) {
    app.exit(e);
    std::ignore = request_termination();
    return;
  }

  auto video_info = load_video(input_filename);
  if (!video_info.has_value()) {
    spdlog::error("Unable to load video");
    std::ignore = request_termination();
    return;
  }

  broadcast(events::new_video_loaded{.info = video_info.value()});
  frame_decoder_ = std::thread(&video_reader::decode_video, this);
}

video_reader::video_reader(video_reader&& reader) noexcept
    : frame_ctx_{std::move(reader.frame_ctx_)},
      audio_ctx_{std::move(reader.audio_ctx_)},
      frame_decoder_{std::move(reader.frame_decoder_)},
      is_paused_{reader.is_paused_.load(std::memory_order_acquire)} {
  std::swap(format_context_ptr_, reader.format_context_ptr_);
}

video_reader& video_reader::operator=(video_reader&& reader) noexcept {
  std::swap(format_context_ptr_, reader.format_context_ptr_);
  frame_ctx_ = std::move(reader.frame_ctx_);
  audio_ctx_ = std::move(reader.audio_ctx_);
  frame_decoder_ = std::move(reader.frame_decoder_);
  is_paused_ = reader.is_paused_.load(std::memory_order_acquire);
  return *this;
}

void video_reader::handle_termination_signal() noexcept {
  is_terminated_.store(true, std::memory_order_release);
  if (frame_decoder_.joinable()) {
    frame_decoder_.join();
  }
}

std::optional<video_info> video_reader::load_video(
    const std::filesystem::path& filename) noexcept {
  spdlog::info("Reading video file: {}", filename.filename().string());
  if (avformat_open_input(&format_context_ptr_, filename.c_str(), nullptr,
                          nullptr) != 0) {
    spdlog::error("Error reading video file {}", filename.filename().string());
    return std::nullopt;
  }

  auto streams = get_media_streams();
  if (streams.empty()) {
    spdlog::error("Error finding stream information for {}",
                  filename.filename().string());
    return std::nullopt;
  }

  auto video_stream_it = std::ranges::find_if(streams, [](AVStream* stream) {
    return stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO;
  });

  auto audio_stream_it = std::ranges::find_if(streams, [](AVStream* stream) {
    return stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO;
  });

  picture_info picture_info{};
  audio_info audio_info{};

  if (video_stream_it == streams.end()) {
    spdlog::error("Unable to detect video stream for {}",
                  filename.filename().string());
    return std::nullopt;
  } else {
    AVCodecParameters* codec_params_ptr = (*video_stream_it)->codecpar;
    const AVCodec* frame_codec =
        avcodec_find_decoder(codec_params_ptr->codec_id);
    frame_ctx_ = media_context{format_context_ptr_, (*video_stream_it),
                               codec_params_ptr, frame_codec};

    picture_info.format = frame_ctx_.codec().long_name;
    picture_info.fps = (*video_stream_it)->avg_frame_rate;
    picture_info.tbn = (*video_stream_it)->time_base;
    picture_info.width = (*video_stream_it)->codecpar->width;
    picture_info.height = (*video_stream_it)->codecpar->height;
  }

  if (audio_stream_it == streams.end()) {
    spdlog::info("Unable to detect audio stream for {}",
                 filename.filename().string());
    audio_info.has_audio_stream = false;
  } else {
    AVCodecParameters* codec_params_ptr = (*audio_stream_it)->codecpar;
    const AVCodec* audio_codec =
        avcodec_find_decoder(codec_params_ptr->codec_id);
    audio_ctx_ = audio_context{format_context_ptr_, (*audio_stream_it),
                               codec_params_ptr, audio_codec};
    std::string_view audio_format_name{
        av_get_sample_fmt_name(audio_ctx_.codec_ctx().sample_fmt)};
    spdlog::info("Audio format name: {}", audio_format_name);

    audio_info.format = audio_ctx_.codec().long_name;
    audio_info.tbn = (*audio_stream_it)->time_base;
    audio_info.sample_format = audio_ctx_.codec_ctx().sample_fmt;
    audio_info.num_channels = audio_ctx_.codec_ctx().ch_layout.nb_channels;
    audio_info.frequency = audio_ctx_.codec_ctx().sample_rate;
    audio_info.has_audio_stream = true;
  }

  return std::optional<video_info>{std::in_place,
                                   format_context_ptr_->iformat->long_name,
                                   picture_info, audio_info};
}

std::span<AVStream*> video_reader::get_media_streams() const noexcept {
  if (avformat_find_stream_info(format_context_ptr_, nullptr) < 0) {
    return {};
  }

  return std::span<AVStream*>{
      format_context_ptr_->streams,
      static_cast<size_t>(format_context_ptr_->nb_streams)};
}

[[nodiscard]] int64_t video_reader::next_seek_request() noexcept {
  auto next_seek_request = seek_request_.load(std::memory_order_acquire);

  // NOTE: Here, we do a CAS operation to make sure that we got the most
  // updated seek request
  while (!seek_request_.compare_exchange_weak(next_seek_request, -1,
                                              std::memory_order_relaxed)) {
  }
  return next_seek_request;
}

[[nodiscard]] video_reader::seek_result
video_reader::process_seek_request() noexcept {
  auto curr_seek_request = next_seek_request();
  if (curr_seek_request == -1) {
    return seek_result::no_seek_request_found;
  }
  if (av_seek_frame(format_context_ptr_, -1, curr_seek_request,
                    AVSEEK_FLAG_BACKWARD) < 0) {
    return seek_result::seek_error;
  }

  audio_ctx_.flush_codec_context();
  frame_ctx_.flush_codec_context();

  return seek_result::seek_request_processed;
}

void video_reader::picture_frame_handler(AVFrame* picture_frame,
                                         bool reset_frame_sequence) noexcept {
  bool requires_dealloc = false;
  if (picture_frame->format != AVPixelFormat::AV_PIX_FMT_YUV420P) {
    picture_frame = utils::convert_frame(picture_frame, AV_PIX_FMT_YUV420P);
    requires_dealloc = true;
  }
  if (is_terminated_.load(std::memory_order_acquire)) [[unlikely]] {
    return;
  }
  event_handler_t::broadcast(
      events::new_frame_loaded{.frame = yuv_frame{picture_frame},
                               .frame_num = frame_ctx_.codec_ctx().frame_num,
                               .frame_pts = picture_frame->pts,
                               .frame_pkt_dts = picture_frame->pkt_dts,
                               .reset_frame_sequence = reset_frame_sequence});
  if (requires_dealloc) {
    av_frame_free(&picture_frame);
  }
}

void video_reader::audio_frame_handler([[maybe_unused]] AVFrame* audio_frame,
                                       bool reset_frame_sequence) noexcept {
  auto& audio_codec_ctx = audio_ctx_.codec_ctx();

  std::vector<uint8_t> audio_buffer;
  if (av_sample_fmt_is_planar(
          static_cast<AVSampleFormat>(audio_frame->format)) == 1) {
    audio_buffer = audio_ctx_.pack_planar_audio_sample(audio_frame);
  } else {
    const auto num_samples = audio_frame->nb_samples;
    const auto sample_size = static_cast<size_t>(
        av_get_bytes_per_sample(audio_codec_ctx.sample_fmt));
    const auto buffer_size =
        sample_size * num_samples * audio_codec_ctx.ch_layout.nb_channels;
    audio_buffer.resize(buffer_size);
    std::memcpy(audio_buffer.data(), audio_frame->data[0], audio_buffer.size());
  }
  if (is_terminated_.load(std::memory_order_acquire)) {
    return;
  }
  event_handler_t::broadcast(events::new_audio_samples_loaded{
      .samples = std::move(audio_buffer),
      .frame_num = audio_codec_ctx.frame_num,
      .reset_frame_sequence = reset_frame_sequence});
}

void video_reader::operator()(const seek_request_event& event) {
  const auto direction = event.payload().direction;
  const auto timebase = frame_ctx_.stream().time_base;

  int32_t multiplier = (direction == seek_direction::backward ? -1 : 1);
  const int64_t delta_pts = seek_unit_secs * multiplier * AV_TIME_BASE;

  auto most_recent_pts_ts =
      av_rescale_q(frame_ctx_.most_recent_pts(), timebase,
                   AVRational{.num = 1, .den = AV_TIME_BASE});

  int64_t curr_seek_pos = seek_request_.load(std::memory_order_relaxed);
  int64_t new_seek_pos = std::clamp(
      (curr_seek_pos == -1 ? most_recent_pts_ts : curr_seek_pos) + delta_pts,
      0L, format_context_ptr_->duration);

  while (!seek_request_.compare_exchange_weak(curr_seek_pos, new_seek_pos,
                                              std::memory_order_relaxed)) {
    most_recent_pts_ts =
        av_rescale_q(frame_ctx_.most_recent_pts(), timebase,
                     AVRational{.num = 1, .den = AV_TIME_BASE});
    new_seek_pos = std::clamp(
        (curr_seek_pos == -1 ? most_recent_pts_ts : curr_seek_pos) + delta_pts,
        0L, format_context_ptr_->duration);
  }
}

void video_reader::decode_video() noexcept {
  av_packet packet{av_packet_alloc()};
  av_frame frame{av_frame_alloc()};

  std::array<media_context*, 2> media_contexts{&frame_ctx_, &audio_ctx_};
  std::array<bool, 2> reset_frame_seq{true, true};

  // NOTE: use std::vector here for auto type deduction
  std::vector frame_handlers{
      std::bind_front(&video_reader::picture_frame_handler, this),
      std::bind_front(&video_reader::audio_frame_handler, this)};

  const auto audio_stream_index = av_find_best_stream(
      format_context_ptr_, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

  while (!is_terminated_.load(std::memory_order_acquire)) {
    const auto seek_result = process_seek_request();
    if (seek_result == seek_result::seek_request_processed) {
      reset_frame_seq[0] = reset_frame_seq[1] = true;
    }

    if (seek_result == seek_result::seek_error) {
      spdlog::error("Error processing seek request");
      std::ignore = request_termination();
      return;
    }

    if (is_paused_.load(std::memory_order_acquire) ||
        av_read_frame(format_context_ptr_, packet.get()) < 0) {
      continue;
    }

    const auto context_index =
        static_cast<size_t>(packet->stream_index == audio_stream_index);
    auto& codec_ctx = media_contexts.at(context_index)->codec_ctx();

    auto ret = avcodec_send_packet(&codec_ctx, packet.get());
    while (ret >= 0) {
      if (is_terminated_.load(std::memory_order_acquire)) {
        return;
      }
      if (is_paused_.load(std::memory_order_acquire)) {
        continue;
      }
      ret = avcodec_receive_frame(&codec_ctx, frame.get());
      if (ret == AVERROR(EAGAIN)) {
        break;
      }
      frame_handlers.at(context_index)(frame.get(),
                                       reset_frame_seq.at(context_index));
      reset_frame_seq.at(context_index) = false;
      media_contexts.at(context_index)->update_most_recent_pts(frame->pts);
    }
  }
}

video_reader::~video_reader() noexcept {
  if (frame_decoder_.joinable()) {
    frame_decoder_.join();
  }
  if (format_context_ptr_ != nullptr) {
    avformat_free_context(format_context_ptr_);
    format_context_ptr_ = nullptr;
  }
}
}  // namespace mvplayer
