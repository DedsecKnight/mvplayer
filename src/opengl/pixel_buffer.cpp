#include "pixel_buffer.hpp"

#include <glad/gl.h>

#include <cstring>
#include <utility>

namespace opengl {
pixel_buffer::pixel_buffer() { glGenBuffers(1, &id_); }

pixel_buffer::pixel_buffer(pixel_buffer&& other_buffer) noexcept
    : id_{std::exchange(other_buffer.id_, 0)} {}

pixel_buffer& pixel_buffer::operator=(pixel_buffer&& other_buffer) noexcept {
  id_ = std::exchange(other_buffer.id_, 0);
  return *this;
}

pixel_buffer::~pixel_buffer() { glDeleteBuffers(1, &id_); }

void pixel_buffer::bind() const noexcept {
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, id_);
}

void pixel_buffer::update_pixel(void* pixel_buffer,
                                GLsizeiptr pixel_buffer_size) const noexcept {
  bind();
  glBufferData(GL_PIXEL_UNPACK_BUFFER, pixel_buffer_size, nullptr,
               GL_STREAM_DRAW);
  void* ptr = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
  std::memcpy(ptr, pixel_buffer, pixel_buffer_size);
  glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

  // Unbind buffer after update
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

[[nodiscard]] GLuint pixel_buffer::id() const noexcept { return id_; }

}  // namespace opengl
