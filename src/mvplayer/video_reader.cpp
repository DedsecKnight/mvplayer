#include "video_reader.hpp"

#include <libavcodec/avcodec.h>
#include <libavutil/pixfmt.h>

#include <atomic>
#include <filesystem>

#include "CLI/CLI.hpp"
#include "events.hpp"
#include "info.hpp"
#include "utils/conversion.hpp"
#include "utils/owned.hpp"

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
    : frame_decoder_{std::move(reader.frame_decoder_)},
      is_paused_{reader.is_paused_.load(std::memory_order_relaxed)} {
  std::swap(format_context_ptr_, reader.format_context_ptr_);
  std::swap(codec_context_ptr_, reader.codec_context_ptr_);
}

video_reader& video_reader::operator=(video_reader&& reader) noexcept {
  std::swap(format_context_ptr_, reader.format_context_ptr_);
  std::swap(codec_context_ptr_, reader.codec_context_ptr_);
  frame_decoder_ = std::move(reader.frame_decoder_);
  is_paused_ = reader.is_paused_.load(std::memory_order_relaxed);
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

  if (video_stream_it == streams.end()) {
    spdlog::error("Unable to detect video stream for {}",
                  filename.filename().string());
    return std::nullopt;
  }

  AVCodecParameters* codec_params_ptr = (*video_stream_it)->codecpar;
  const AVCodec* codec_ptr = avcodec_find_decoder(codec_params_ptr->codec_id);

  if (!load_codec_context(codec_ptr, codec_params_ptr)) {
    spdlog::error("Error loading context for codec {}", codec_ptr->long_name);
    return std::nullopt;
  }

  return std::optional<video_info>{std::in_place,
                                   format_context_ptr_->iformat->long_name,
                                   codec_ptr->long_name,
                                   (*video_stream_it)->avg_frame_rate,
                                   (*video_stream_it)->time_base,
                                   codec_params_ptr->bit_rate,
                                   format_context_ptr_->duration,
                                   codec_params_ptr->width,
                                   codec_params_ptr->height};
}

std::span<AVStream*> video_reader::get_media_streams() const noexcept {
  if (avformat_find_stream_info(format_context_ptr_, nullptr) < 0) {
    return {};
  }

  return std::span<AVStream*>{
      format_context_ptr_->streams,
      static_cast<size_t>(format_context_ptr_->nb_streams)};
}

bool video_reader::load_codec_context(
    const AVCodec* codec_ptr, AVCodecParameters* codec_params_ptr) noexcept {
  codec_context_ptr_ = avcodec_alloc_context3(codec_ptr);
  if (avcodec_parameters_to_context(codec_context_ptr_, codec_params_ptr) < 0) {
    return false;
  }
  if (avcodec_open2(codec_context_ptr_, codec_ptr, nullptr) < 0) {
    return false;
  }
  return true;
}

void video_reader::decode_video() noexcept {
  av_packet packet{av_packet_alloc()};
  av_frame frame{av_frame_alloc()};
  while (!is_terminated_.load(std::memory_order_relaxed)) {
    if (is_paused_.load(std::memory_order_relaxed) ||
        av_read_frame(format_context_ptr_, packet.get()) < 0) {
      continue;
    }
    avcodec_send_packet(codec_context_ptr_, packet.get());
    avcodec_receive_frame(codec_context_ptr_, frame.get());

    auto converted_frame =
        utils::convert_frame(frame.get(), AVPixelFormat::AV_PIX_FMT_RGB24);

    cv::Mat frame_mat(converted_frame->height, converted_frame->width, CV_8UC3);
    std::memcpy(frame_mat.data, converted_frame->data[0],
                frame_mat.elemSize() * frame_mat.total());
    if (is_terminated_.load(std::memory_order_acquire)) {
      break;
    }
    event_handler_t::broadcast(
        events::new_frame_loaded{.frame = frame_mat,
                                 .frame_num = codec_context_ptr_->frame_num,
                                 .frame_pts = frame->pts,
                                 .frame_pkt_dts = frame->pkt_dts});
  }
}

video_reader::~video_reader() noexcept {
  if (frame_decoder_.joinable()) {
    frame_decoder_.join();
  }
  if (codec_context_ptr_ != nullptr) {
    avcodec_free_context(&codec_context_ptr_);
  }
  if (format_context_ptr_ != nullptr) {
    avformat_free_context(format_context_ptr_);
    format_context_ptr_ = nullptr;
  }
}
}  // namespace mvplayer
