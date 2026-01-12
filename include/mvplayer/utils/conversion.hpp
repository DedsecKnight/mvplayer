#pragma once

#include "utils/owned.hpp"
namespace mvplayer::utils {
av_frame convert_frame(AVFrame* src_frame, AVPixelFormat dst_format);
}  // namespace mvplayer::utils
