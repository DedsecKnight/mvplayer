#include "renderer/yuvp8/renderer.hpp"

#include <glad/gl.h>

#include "renderer/yuvp8/shader_spec.hpp"

namespace mvplayer::renderer::yuvp8 {
renderer::renderer() : base(vertex_shader_spec{}, fragment_shader_spec{}) {}

[[nodiscard]] bool renderer::render_frame(const AVFrame* frame) noexcept {
  bool half_width = frame->format == AV_PIX_FMT_YUV420P ||
                    frame->format == AV_PIX_FMT_YUV422P;
  bool half_height = frame->format == AV_PIX_FMT_YUV420P;

  std::array<int32_t, 3> plane_width{
      frame->width, half_width ? frame->width / 2 : frame->width,
      half_width ? frame->width / 2 : frame->width};
  std::array<int32_t, 3> plane_height{
      frame->height, half_height ? frame->height / 2 : frame->height,
      half_height ? frame->height / 2 : frame->height};
  std::span<uint8_t* const> plane_view{frame->data, 3UZ};        // NOLINT
  std::span<const int32_t> linesize_view{frame->linesize, 3UZ};  // NOLINT

  for (auto i{3UZ}; i-- > 0;) {
    std::span<uint8_t> plane_data_view{
        plane_view[i],
        static_cast<size_t>(plane_height.at(i) * linesize_view[i])};
    glPixelStorei(GL_UNPACK_ROW_LENGTH,
                  static_cast<int32_t>(linesize_view[i] / sizeof(uint8_t)));
    plane_textures_.at(i).configure_texture_data(
        plane_data_view, GL_RED,
        {.width = plane_width.at(i), .height = plane_height.at(i)});
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    plane_textures_.at(i).bind(i);
  }
  draw_frame();
  return true;
}
}  // namespace mvplayer::renderer::yuvp8
