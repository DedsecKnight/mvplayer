#include "renderer/factory.hpp"

#include <algorithm>

#include "renderer/base.hpp"
#include "renderer/rgb/renderer.hpp"

namespace mvplayer::renderer {
[[nodiscard]] std::unique_ptr<base> factory::create_renderer_pipeline(
    AVPixelFormat format) noexcept {
  switch (format) {
    case AV_PIX_FMT_RGB24:
      return std::make_unique<rgb::renderer>();
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
