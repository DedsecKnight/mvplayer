#pragma once

#include <glad/gl.h>

#include <span>

namespace opengl {
class vertex_buffer {
 private:
  using draw_strategy_t = uint32_t;

 public:
  // Create a vertex buffer on GPU. Requires explicit binding to be able to use
  vertex_buffer();

  // Create a vertex buffer on GPU with data loaded. Buffer will be binded after
  // invocation
  explicit vertex_buffer(std::span<const float> vertices_data,
                         draw_strategy_t draw_strategy = GL_STATIC_DRAW);

  void bind() const noexcept;

  [[nodiscard]] GLuint id() const noexcept;

  vertex_buffer(const vertex_buffer&) = delete;
  vertex_buffer& operator=(const vertex_buffer&) = delete;

  vertex_buffer(vertex_buffer&&) noexcept;
  vertex_buffer& operator=(vertex_buffer&&) noexcept;

  ~vertex_buffer();

 private:
  GLuint id_{0};
};
}  // namespace opengl
