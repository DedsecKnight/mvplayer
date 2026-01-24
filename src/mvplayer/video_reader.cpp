#include "video_reader.hpp"

#include <libavutil/samplefmt.h>

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
}

#include <spdlog/spdlog.h>

#include <opencv2/imgproc.hpp>

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
    frame_ctx_ = media_context{codec_params_ptr, frame_codec};

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
    audio_ctx_ = audio_context{codec_params_ptr, audio_codec};
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

void video_reader::picture_frame_handler(AVFrame* picture_frame) noexcept {
  auto converted_frame =
      utils::convert_frame(picture_frame, AVPixelFormat::AV_PIX_FMT_RGB24);

  cv::Mat frame_mat(converted_frame->height, converted_frame->width, CV_8UC3);
  std::memcpy(frame_mat.data, converted_frame->data[0],
              frame_mat.elemSize() * frame_mat.total());
  event_handler_t::broadcast(
      events::new_frame_loaded{.frame = frame_mat,
                               .frame_num = frame_ctx_.codec_ctx().frame_num,
                               .frame_pts = picture_frame->pts,
                               .frame_pkt_dts = picture_frame->pkt_dts});
}

void video_reader::audio_frame_handler(
    [[maybe_unused]] AVFrame* audio_frame) noexcept {
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

  event_handler_t::broadcast(
      events::new_audio_samples_loaded{.samples = std::move(audio_buffer),
                                       .frame_num = audio_codec_ctx.frame_num,
                                       .frame_pts = audio_frame->pts,
                                       .frame_pkt_dts = audio_frame->pkt_dts});
}

void video_reader::decode_video() noexcept {
  av_packet packet{av_packet_alloc()};
  av_frame frame{av_frame_alloc()};

  auto& frame_codec_ctx = frame_ctx_.codec_ctx();
  auto& audio_codec_ctx = audio_ctx_.codec_ctx();

  const auto audio_stream_index = av_find_best_stream(
      format_context_ptr_, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

  while (!is_terminated_.load(std::memory_order_acquire)) {
    if (is_paused_.load(std::memory_order_acquire) ||
        av_read_frame(format_context_ptr_, packet.get()) < 0) {
      continue;
    }
    bool is_audio_packet = packet->stream_index == audio_stream_index;
    auto* codec_ctx_ptr =
        (is_audio_packet ? &audio_codec_ctx : &frame_codec_ctx);

    auto ret = avcodec_send_packet(codec_ctx_ptr, packet.get());
    while (ret >= 0) {
      if (is_terminated_.load(std::memory_order_acquire)) {
        return;
      }
      if (is_paused_.load(std::memory_order_acquire)) {
        continue;
      }
      ret = avcodec_receive_frame(codec_ctx_ptr, frame.get());
      if (ret == AVERROR(EAGAIN)) {
        break;
      }
      if (is_audio_packet) {
        audio_frame_handler(frame.get());
      } else {
        picture_frame_handler(frame.get());
      }
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
