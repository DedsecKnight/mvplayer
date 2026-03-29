#include "renderer/channel_plane.hpp"

#include <glad/gl.h>

#include "texture.hpp"

namespace mvplayer::renderer {

[[nodiscard]] std::expected<void, error> channel_plane::load_plane(
    std::span<uint8_t> plane_data, const plane_data_spec& spec) noexcept {
  const auto read_index = (pb_index_ ^= 1U);
  const auto write_index = read_index ^ 1U;

  texture_.bind(spec.texture_slot);

  GL_INVOKE(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR),
            "set texture min filter");
  GL_INVOKE(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR),
            "set texture mag filter");

  pixel_buffer_.at(read_index).bind();

  if (!initialized_) {
    GL_INVOKE(glBufferData(GL_PIXEL_UNPACK_BUFFER,
                           static_cast<GLsizeiptr>(plane_data.size()), nullptr,
                           GL_STREAM_DRAW),
              "initialize pixel buffer");
    GL_INVOKE(
        glTexImage2D(GL_TEXTURE_2D, 0, spec.internal_format, spec.width,
                     spec.height, 0, spec.format, spec.data_type, nullptr),
        "load pixel buffer to texture");
    initialized_ = true;
  } else {
    GL_INVOKE(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, spec.width, spec.height,
                              spec.format, spec.data_type, nullptr),
              "load pixel buffer to texture");
  }

  GL_INVOKE(pixel_buffer_.at(write_index)
                .update_pixel(plane_data.data(),
                              static_cast<GLsizeiptr>(plane_data.size())),
            "update pixel buffer data");

  return {};
}

}  // namespace mvplayer::renderer
