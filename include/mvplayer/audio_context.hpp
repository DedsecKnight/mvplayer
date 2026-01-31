#pragma once

#include <vector>

#include "media_context.hpp"
#include "utils/owned.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
}

namespace mvplayer {

class audio_context : public media_context {
 public:
  audio_context() = default;
  audio_context(AVFormatContext* format_ctx_ptr, AVStream* stream,
                AVCodecParameters* codec_params_ptr, const AVCodec* codec_ptr)
      : media_context(format_ctx_ptr, stream, codec_params_ptr, codec_ptr) {}
  [[nodiscard]] std::vector<uint8_t> pack_planar_audio_sample(
      AVFrame* audio_frame) const noexcept;

 private:
  [[nodiscard]] swr_context create_swr_context(
      AVSampleFormat output_fmt) const noexcept;
};
}  // namespace mvplayer
