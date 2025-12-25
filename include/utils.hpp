#pragma once

#include <memory>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

namespace mvplayer {
namespace utils {
void avFrameDeallocator(AVFrame* frame);
void avPacketDeallocator(AVPacket* packet);

std::unique_ptr<AVFrame, decltype(&avFrameDeallocator)> convertFrame(
    AVFrame* srcFrame, AVPixelFormat destFormat);

}  // namespace utils
}  // namespace mvplayer
