#pragma once

extern "C" {
#include <libavutil/pixfmt.h>
}

#include <memory>

#include "renderer/base.hpp"
namespace mvplayer::renderer {
class factory {
 private:
  static constexpr std::array SUPPORTED_PIX_FMT = {
      AV_PIX_FMT_RGB24,       AV_PIX_FMT_YUV422P,     AV_PIX_FMT_YUV420P,
      AV_PIX_FMT_YUV440P,     AV_PIX_FMT_YUV444P,     AV_PIX_FMT_YUV422P10LE,
      AV_PIX_FMT_YUV420P10LE, AV_PIX_FMT_YUV440P10LE, AV_PIX_FMT_YUV444P10LE};

 public:
  [[nodiscard]] static std::unique_ptr<base> create_renderer_pipeline(
      AVPixelFormat format) noexcept;
  [[nodiscard]] static bool supports_format(AVPixelFormat format) noexcept;
};
};  // namespace mvplayer::renderer
