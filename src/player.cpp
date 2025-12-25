#include "player.hpp"

#include <filesystem>

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

  if (avformat_find_stream_info(pFormatContext_, nullptr)) {
    spdlog::error("Error finding stream information for {}",
                  filename.filename().string());
    return std::nullopt;
  }

  std::span<AVStream*> streams{
      pFormatContext_->streams,
      pFormatContext_->streams + pFormatContext_->nb_streams};

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

  return VideoInfo{pFormatContext_->iformat->long_name,
                   pCodec->long_name,
                   pCodecParams->bit_rate,
                   pFormatContext_->duration,
                   pCodecParams->width,
                   pCodecParams->height};
}

VideoPlayer::~VideoPlayer() noexcept {
  if (pFormatContext_ != nullptr) {
    avformat_free_context(pFormatContext_);
    pFormatContext_ = nullptr;
  }
}
}  // namespace mvplayer
