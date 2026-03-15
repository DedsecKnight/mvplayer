#include "renderer/yuvp/renderer.hpp"

#include <glad/gl.h>
#include <libavutil/pixfmt.h>
#include <spdlog/spdlog.h>

#include "renderer/yuvp/shader_spec.hpp"

namespace mvplayer::renderer::yuvp {
renderer::renderer()
    : base(vertex_shader_spec{}, fragment_shader_spec{}), planes_{{}} {}

bool renderer::pix_fmt_is_10bit(AVPixelFormat pixel_format) noexcept {
  return pixel_format == AV_PIX_FMT_YUV420P10LE ||
         pixel_format == AV_PIX_FMT_YUV444P10LE ||
         pixel_format == AV_PIX_FMT_YUV422P10LE;
}

[[nodiscard]] bool renderer::render_frame(const AVFrame* frame) noexcept {
  bool half_width = frame->format == AV_PIX_FMT_YUV420P ||
                    frame->format == AV_PIX_FMT_YUV422P ||
                    frame->format == AV_PIX_FMT_YUV422P10LE ||
                    frame->format == AV_PIX_FMT_YUV420P10LE;
  bool half_height = frame->format == AV_PIX_FMT_YUV420P ||
                     frame->format == AV_PIX_FMT_YUV420P10LE;

  std::array<int32_t, 3> plane_width{
      frame->width, half_width ? frame->width / 2 : frame->width,
      half_width ? frame->width / 2 : frame->width};
  std::array<int32_t, 3> plane_height{
      frame->height, half_height ? frame->height / 2 : frame->height,
      half_height ? frame->height / 2 : frame->height};
  std::span<uint8_t* const> plane_view{frame->data, 3UZ};        // NOLINT
  std::span<const int32_t> linesize_view{frame->linesize, 3UZ};  // NOLINT

  bool is_10_bit = pix_fmt_is_10bit(static_cast<AVPixelFormat>(frame->format));
  const auto multiplier = is_10_bit ? 64.0F : 1.0F;
  glProgramUniform1f(shader_.id(),
                     glGetUniformLocation(shader_.id(), "multiplier"),
                     multiplier);

  for (auto i{3UZ}; i-- > 0;) {
    std::span<uint8_t> plane_data_view{
        plane_view[i],
        static_cast<size_t>(plane_height.at(i) * linesize_view[i])};
    auto pixel_value_size = is_10_bit ? sizeof(uint16_t) : sizeof(uint8_t);
    glPixelStorei(GL_UNPACK_ROW_LENGTH,
                  static_cast<int32_t>(linesize_view[i] / pixel_value_size));
    if (!planes_.at(i).load_plane(
            plane_data_view,
            {.internal_format = is_10_bit ? GL_R16 : GL_RED,
             .format = GL_RED,
             .data_type = static_cast<GLenum>(is_10_bit ? GL_UNSIGNED_SHORT
                                                        : GL_UNSIGNED_BYTE),
             .texture_slot = static_cast<uint32_t>(i),
             .width = plane_width.at(i),
             .height = plane_height.at(i)})) {
      glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
      return false;
    }
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  }
  draw_frame();
  return true;
}
}  // namespace mvplayer::renderer::yuvp
