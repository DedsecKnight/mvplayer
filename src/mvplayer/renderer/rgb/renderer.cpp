#include "renderer/rgb/renderer.hpp"

#include <glad/gl.h>

#include <cstring>

#include "renderer/rgb/shader_spec.hpp"
#include "texture.hpp"

namespace mvplayer::renderer::rgb {
renderer::renderer() : base(vertex_shader_spec{}, fragment_shader_spec{}) {}

[[nodiscard]] bool renderer::render_frame(const AVFrame* frame) noexcept {
  constexpr int pixel_size = 3 * sizeof(uint8_t);
  std::span<uint8_t> frame_data_view{
      frame->data[0], static_cast<size_t>(frame->linesize[0] * frame->height)};

  glPixelStorei(GL_UNPACK_ROW_LENGTH, frame->linesize[0] / pixel_size);
  texture_.configure_texture_data(
      frame_data_view, GL_RGB,
      {.width = frame->width, .height = frame->height});
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  draw_frame();
  return true;
}
}  // namespace mvplayer::renderer::rgb
