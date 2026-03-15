#include "index_buffer.hpp"

#include <glad/gl.h>

#include <utility>

namespace opengl {
index_buffer::index_buffer() { glGenBuffers(1, &id_); }
index_buffer::index_buffer(std::span<const uint32_t> indices) : index_buffer() {
  bind();
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               static_cast<int64_t>(indices.size() * sizeof(uint32_t)),
               indices.data(), GL_STATIC_DRAW);
}

void index_buffer::bind() const noexcept {
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id_);
}

index_buffer::index_buffer(index_buffer&& other_buffer) noexcept
    : id_{std::exchange(other_buffer.id_, 0)} {}

index_buffer& index_buffer::operator=(index_buffer&& other_buffer) noexcept {
  id_ = std::exchange(other_buffer.id_, 0);
  return *this;
}

GLuint index_buffer::id() const noexcept { return id_; }

index_buffer::~index_buffer() { glDeleteBuffers(1, &id_); }
}  // namespace opengl
