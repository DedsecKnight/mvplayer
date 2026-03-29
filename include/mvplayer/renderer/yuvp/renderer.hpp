#pragma once

#include <libavutil/pixfmt.h>

#include "renderer/base.hpp"
#include "renderer/channel_plane.hpp"
namespace mvplayer::renderer::yuvp {

class renderer : public base {
 public:
  renderer();

  renderer(const renderer&) = delete;
  renderer& operator=(const renderer&) = delete;

  renderer(renderer&&) noexcept = default;
  renderer& operator=(renderer&&) noexcept = default;

  ~renderer() override = default;

  [[nodiscard]] std::expected<void, error> render_frame(
      const AVFrame* frame) noexcept override;

 private:
  static bool pix_fmt_is_10bit(AVPixelFormat pixel_format) noexcept;
  std::expected<void, error> update_plane_data(
      size_t plane_index, std::span<uint8_t> plane_data,
      int32_t unpack_row_length, const plane_data_spec& spec) noexcept;

  std::array<channel_plane, 3> planes_;
};
}  // namespace mvplayer::renderer::yuvp
