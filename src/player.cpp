#include "player.hpp"

#include <libavcodec/avcodec.h>
#include <libavcodec/codec_par.h>

#include <filesystem>

#include "info.hpp"

extern "C" {
#include <libavcodec/codec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

#include <spdlog/spdlog.h>

namespace mvplayer {
std::optional<VideoInfo> VideoPlayer::loadVideo(
    const std::filesystem::path& filename) noexcept {
  spdlog::info("Reading video file: {}", filename.filename().string());
  if (avformat_open_input(&pFormatContext_, filename.c_str(), nullptr,
                          nullptr)) {
    spdlog::error("Error reading video file {}", filename.filename().string());
    return std::nullopt;
  }

  auto streams = getMediaStreams();
  if (streams.empty()) {
    spdlog::error("Error finding stream information for {}",
                  filename.filename().string());
    return std::nullopt;
  }

  auto videoStreamIt = std::ranges::find_if(streams, [](AVStream* stream) {
    return stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO;
  });

  if (videoStreamIt == streams.end()) {
    spdlog::error("Unable to detect video stream for {}",
                  filename.filename().string());
    return std::nullopt;
  }

  AVCodecParameters* pCodecParams = (*videoStreamIt)->codecpar;
  const AVCodec* pCodec = avcodec_find_decoder(pCodecParams->codec_id);

  if (!loadCodecContext(pCodec, pCodecParams)) {
    spdlog::error("Error loading context for codec {}", pCodec->long_name);
    return std::nullopt;
  }

  return VideoInfo{pFormatContext_->iformat->long_name,
                   pCodec->long_name,
                   pCodecParams->bit_rate,
                   pFormatContext_->duration,
                   pCodecParams->width,
                   pCodecParams->height};
}

std::optional<FrameInfo> VideoPlayer::getFrame() const noexcept {
  AVPacket* pPacket = av_packet_alloc();
  AVFrame* pFrame = av_frame_alloc();
  if (av_read_frame(pFormatContext_, pPacket) < 0) {
    return std::nullopt;
  }
  avcodec_send_packet(pCodecContext_, pPacket);
  avcodec_receive_frame(pCodecContext_, pFrame);
  return FrameInfo{pFrame->pts, pFrame->pkt_dts};
}

std::span<AVStream*> VideoPlayer::getMediaStreams() const noexcept {
  if (avformat_find_stream_info(pFormatContext_, nullptr)) {
    return {};
  }

  return std::span<AVStream*>{
      pFormatContext_->streams,
      pFormatContext_->streams + pFormatContext_->nb_streams};
}

bool VideoPlayer::loadCodecContext(const AVCodec* pCodec,
                                   AVCodecParameters* pCodecParams) noexcept {
  pCodecContext_ = avcodec_alloc_context3(pCodec);
  if (avcodec_parameters_to_context(pCodecContext_, pCodecParams) < 0) {
    return false;
  }
  if (avcodec_open2(pCodecContext_, pCodec, nullptr) < 0) {
    return false;
  }
  return true;
}

VideoPlayer::~VideoPlayer() noexcept {
  if (pCodecContext_ != nullptr) {
    avcodec_free_context(&pCodecContext_);
  }
  if (pFormatContext_ != nullptr) {
    avformat_free_context(pFormatContext_);
    pFormatContext_ = nullptr;
  }
}
}  // namespace mvplayer
