#include "utils/conversion.hpp"

#include "utils/owned.hpp"

extern "C" {
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

namespace mvplayer::utils {
av_frame convert_frame(AVFrame* src_frame, AVPixelFormat dst_format) {
  av_frame dest_frame{av_frame_alloc()};
  if (dest_frame == nullptr) {
    return dest_frame;
  }

  if (av_image_alloc(dest_frame->data, dest_frame->linesize, src_frame->width,
                     src_frame->height, dst_format, 1) < 0) {
    dest_frame.reset(nullptr);
    return dest_frame;
  }

  dest_frame->width = src_frame->width;
  dest_frame->height = src_frame->height;
  dest_frame->format = dst_format;

  SwsContext* conversionContext =
      sws_getContext(src_frame->width, src_frame->height,
                     static_cast<AVPixelFormat>(src_frame->format),
                     dest_frame->width, dest_frame->height, dst_format,
                     SWS_FAST_BILINEAR | SWS_FULL_CHR_H_INT | SWS_ACCURATE_RND,
                     nullptr, nullptr, nullptr);
  sws_scale(conversionContext, src_frame->data, src_frame->linesize, 0,
            src_frame->height, dest_frame->data, dest_frame->linesize);
  sws_freeContext(conversionContext);
  return dest_frame;
}
}  // namespace mvplayer::utils
