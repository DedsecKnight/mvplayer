#include "vertex_buffer.hpp"

#include <glad/gl.h>

#include <utility>

namespace opengl {
vertex_buffer::vertex_buffer() { glGenBuffers(1, &id_); }

vertex_buffer::vertex_buffer(std::span<const float> vertices_data,
                             draw_strategy_t draw_strategy)
    : vertex_buffer() {
  bind();
  glBufferData(GL_ARRAY_BUFFER,
               static_cast<int64_t>(vertices_data.size() * sizeof(float)),
               vertices_data.data(), draw_strategy);
}

void vertex_buffer::bind() const noexcept {
  glBindBuffer(GL_ARRAY_BUFFER, id_);
}

vertex_buffer::vertex_buffer(vertex_buffer&& buffer) noexcept
    : id_{std::exchange(buffer.id_, 0)} {}

vertex_buffer& vertex_buffer::operator=(vertex_buffer&& buffer) noexcept {
  id_ = std::exchange(buffer.id_, 0);
  return *this;
}

GLuint vertex_buffer::id() const noexcept { return id_; }

vertex_buffer::~vertex_buffer() { glDeleteBuffers(1, &id_); }

};  // namespace opengl
