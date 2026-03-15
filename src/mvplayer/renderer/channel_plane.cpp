#include "renderer/channel_plane.hpp"

#include <glad/gl.h>

#include "texture.hpp"

namespace mvplayer::renderer {

[[nodiscard]] bool channel_plane::load_plane(
    std::span<uint8_t> plane_data, const plane_data_spec& spec) noexcept {
  auto read_index = (pb_index_ + 1) % 2;
  auto write_index = (read_index + 1) % 2;

  texture_.bind(spec.texture_slot);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  pixel_buffer_.at(read_index).bind();

  if (!initialized_) {
    glBufferData(GL_PIXEL_UNPACK_BUFFER,
                 static_cast<GLsizeiptr>(plane_data.size()), nullptr,
                 GL_STREAM_DRAW);
    glTexImage2D(GL_TEXTURE_2D, 0, spec.internal_format, spec.width,
                 spec.height, 0, spec.format, spec.data_type, nullptr);
    initialized_ = true;
  } else {
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, spec.width, spec.height,
                    spec.format, spec.data_type, nullptr);
  }

  pixel_buffer_.at(write_index)
      .update_pixel(plane_data.data(),
                    static_cast<GLsizeiptr>(plane_data.size()));

  pb_index_ = (pb_index_ + 1) % 2;
  return true;
}

}  // namespace mvplayer::renderer
