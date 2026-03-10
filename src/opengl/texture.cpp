#include "texture.hpp"

#include <glad/gl.h>

#include <utility>

namespace opengl {
texture::texture() { glGenTextures(1, &id_); }

texture::texture(std::span<uint8_t> pixel_buffer, GLint pixel_format,
                 const texture_spec& spec)
    : texture() {
  configure_texture_data(pixel_buffer, pixel_format, spec);
}

void texture::bind(uint32_t slot) const noexcept {
  glActiveTexture(GL_TEXTURE0 + slot);
  glBindTexture(GL_TEXTURE_2D, id_);
}

texture::texture(texture&& other_texture) noexcept
    : id_{std::exchange(other_texture.id_, 0)},
      initialized_{std::exchange(other_texture.initialized_, false)} {}

texture& texture::operator=(texture&& other_texture) noexcept {
  id_ = std::exchange(other_texture.id_, 0);
  initialized_ = std::exchange(other_texture.initialized_, false);
  return *this;
}

texture::~texture() { glDeleteTextures(1, &id_); }

void texture::configure_texture_data(std::span<uint8_t> pixel_buffer,
                                     GLint pixel_format,
                                     const texture_spec& spec) noexcept {
  bind();
  if (!initialized_) {
    configure_filtering();
    glTexImage2D(GL_TEXTURE_2D, 0, pixel_format, spec.width, spec.height, 0,
                 pixel_format, GL_UNSIGNED_BYTE, pixel_buffer.data());
    initialized_ = true;
  } else {
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, spec.width, spec.height,
                    pixel_format, GL_UNSIGNED_BYTE, pixel_buffer.data());
  }
}

void texture::configure_filtering() const noexcept {
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

}  // namespace opengl
