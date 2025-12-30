#include "video_reader.hpp"

#include <filesystem>

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
std::optional<video_info> video_reader::load_video(
    const std::filesystem::path& filename) noexcept {
  spdlog::info("Reading video file: {}", filename.filename().string());
  if (avformat_open_input(&format_context_ptr_, filename.c_str(), nullptr,
                          nullptr)) {
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
                                   codec_params_ptr->bit_rate,
                                   format_context_ptr_->duration,
                                   codec_params_ptr->width,
                                   codec_params_ptr->height};
}

std::optional<frame_info> video_reader::get_frame() const noexcept {
  av_packet packet{av_packet_alloc()};
  av_frame frame{av_frame_alloc()};

  if (av_read_frame(format_context_ptr_, packet.get()) < 0) {
    return std::nullopt;
  }

  avcodec_send_packet(codec_context_ptr_, packet.get());
  avcodec_receive_frame(codec_context_ptr_, frame.get());

  auto converted_frame =
      utils::convert_frame(frame.get(), AVPixelFormat::AV_PIX_FMT_RGB24);

  cv::Mat frame_mat(converted_frame->height, converted_frame->width, CV_8UC3);
  std::memcpy(frame_mat.data, converted_frame->data[0],
              frame_mat.elemSize() * frame_mat.total());

  return std::optional<frame_info>{std::in_place, frame_mat,
                                   codec_context_ptr_->frame_num, frame->pts,
                                   frame->pkt_dts};
}

std::span<AVStream*> video_reader::get_media_streams() const noexcept {
  if (avformat_find_stream_info(format_context_ptr_, nullptr)) {
    return {};
  }

  return std::span<AVStream*>{
      format_context_ptr_->streams,
      format_context_ptr_->streams + format_context_ptr_->nb_streams};
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

video_reader::~video_reader() noexcept {
  if (codec_context_ptr_ != nullptr) {
    avcodec_free_context(&codec_context_ptr_);
  }
  if (format_context_ptr_ != nullptr) {
    avformat_free_context(format_context_ptr_);
    format_context_ptr_ = nullptr;
  }
}
}  // namespace mvplayer
