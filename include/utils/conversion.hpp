#pragma once

#include "utils/owned.hpp"
namespace mvplayer {
namespace utils {
OwnedAVFrame convertFrame(AVFrame* srcFrame, AVPixelFormat destFormat);
}
}  // namespace mvplayer
