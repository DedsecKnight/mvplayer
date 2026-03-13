#include "renderer/factory.hpp"

#include <libavutil/pixfmt.h>

#include <algorithm>

#include "renderer/base.hpp"
#include "renderer/rgb/renderer.hpp"
#include "renderer/yuvp8/renderer.hpp"

namespace mvplayer::renderer {
[[nodiscard]] std::unique_ptr<base> factory::create_renderer_pipeline(
    AVPixelFormat format) noexcept {
  switch (format) {
    case AV_PIX_FMT_RGB24:
      return std::make_unique<rgb::renderer>();
    case AV_PIX_FMT_YUV420P:
    case AV_PIX_FMT_YUV422P:
    case AV_PIX_FMT_YUV440P:
      return std::make_unique<yuvp8::renderer>();
    default:
      return nullptr;
  }
}

[[nodiscard]] bool factory::supports_format(AVPixelFormat format) noexcept {
  namespace ranges = std::ranges;
  return ranges::find(SUPPORTED_PIX_FMT, format) !=
         std::cend(SUPPORTED_PIX_FMT);
}

}  // namespace mvplayer::renderer
