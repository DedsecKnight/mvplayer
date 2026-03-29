#include "renderer/rgb/renderer.hpp"

#include <glad/gl.h>

#include <cstring>

#include "error.hpp"
#include "renderer/rgb/shader_spec.hpp"

namespace mvplayer::renderer::rgb {
renderer::renderer() : base(vertex_shader_spec{}, fragment_shader_spec{}) {}

[[nodiscard]] std::expected<void, error> renderer::render_frame(
    const AVFrame* frame) noexcept {
  constexpr int pixel_size = 3 * sizeof(uint8_t);
  std::span<uint8_t> frame_data_view{
      frame->data[0], static_cast<size_t>(frame->linesize[0] * frame->height)};

  GL_INVOKE(
      glPixelStorei(GL_UNPACK_ROW_LENGTH, frame->linesize[0] / pixel_size),
      "set GL_UNPACK_ROW_LENGTH for rgb");
  if (auto res =
          plane_.load_plane(frame_data_view, {.internal_format = GL_RGB,
                                              .format = GL_RGB,
                                              .data_type = GL_UNSIGNED_BYTE,
                                              .texture_slot = 0,
                                              .width = frame->width,
                                              .height = frame->height});
      !res.has_value()) {
    GL_INVOKE(glPixelStorei(GL_UNPACK_ROW_LENGTH, 0),
              "reset GL_UNPACK_ROW_LENGTH for rgb");
    return std::unexpected(res.error());
  }
  GL_INVOKE(glPixelStorei(GL_UNPACK_ROW_LENGTH, 0),
            "reset GL_UNPACK_ROW_LENGTH for rgb");
  draw_frame();
  return {};
}
}  // namespace mvplayer::renderer::rgb
