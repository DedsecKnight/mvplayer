#include "utils/conversion.hpp"

#include <SDL3/SDL_audio.h>

#include "utils/owned.hpp"

extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libswscale/swscale.h>
}

namespace mvplayer::utils {

av_frame convert_frame(AVFrame* src_frame, AVPixelFormat dst_format) {
  av_frame dest_frame{av_frame_alloc()};
  if (dest_frame == nullptr) {
    return dest_frame;
  }

  if (av_image_alloc(static_cast<uint8_t**>(dest_frame->data),
                     static_cast<int32_t*>(dest_frame->linesize),
                     src_frame->width, src_frame->height, dst_format, 1) < 0) {
    dest_frame.reset(nullptr);
    return dest_frame;
  }

  dest_frame->width = src_frame->width;
  dest_frame->height = src_frame->height;
  dest_frame->format = dst_format;

  SwsContext* conversion_context = sws_getContext(
      src_frame->width, src_frame->height,
      static_cast<AVPixelFormat>(src_frame->format), dest_frame->width,
      dest_frame->height, dst_format,
      SWS_FAST_BILINEAR | SWS_FULL_CHR_H_INT | SWS_ACCURATE_RND,  // NOLINT
      nullptr, nullptr, nullptr);
  sws_scale(conversion_context, static_cast<uint8_t**>(src_frame->data),
            static_cast<int32_t*>(src_frame->linesize), 0, src_frame->height,
            static_cast<uint8_t**>(dest_frame->data),
            static_cast<int32_t*>(dest_frame->linesize));
  sws_freeContext(conversion_context);
  return dest_frame;
}

SDL_AudioFormat to_sdl_format(AVSampleFormat ff_sample_format) {
  namespace ranges = std::ranges;
  const auto* const mapping_it = ranges::find_if(
      AUDIO_FORMAT_MAPPING, [ff_sample_format](const auto& audio_mapping) {
        return audio_mapping.first == ff_sample_format;
      });
  return mapping_it->second;
}
}  // namespace mvplayer::utils
