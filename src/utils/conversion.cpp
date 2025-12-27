#include "utils/conversion.hpp"

#include "utils/owned.hpp"

extern "C" {
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

namespace mvplayer::utils {
OwnedAVFrame convertFrame(AVFrame* srcFrame, AVPixelFormat dstFormat) {
  OwnedAVFrame destFrame{av_frame_alloc()};
  if (destFrame == nullptr) {
    return destFrame;
  }

  if (av_image_alloc(destFrame->data, destFrame->linesize, srcFrame->width,
                     srcFrame->height, dstFormat, 1) < 0) {
    destFrame.reset(nullptr);
    return destFrame;
  }

  destFrame->width = srcFrame->width;
  destFrame->height = srcFrame->height;
  destFrame->format = dstFormat;

  SwsContext* conversionContext =
      sws_getContext(srcFrame->width, srcFrame->height,
                     static_cast<AVPixelFormat>(srcFrame->format),
                     destFrame->width, destFrame->height, dstFormat,
                     SWS_FAST_BILINEAR | SWS_FULL_CHR_H_INT | SWS_ACCURATE_RND,
                     nullptr, nullptr, nullptr);
  sws_scale(conversionContext, srcFrame->data, srcFrame->linesize, 0,
            srcFrame->height, destFrame->data, destFrame->linesize);
  sws_freeContext(conversionContext);
  return destFrame;
}
}  // namespace mvplayer::utils
