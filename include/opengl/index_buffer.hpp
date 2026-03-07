#pragma once

#include <glad/gl.h>

#include <span>
namespace opengl {
class index_buffer {
 public:
  index_buffer();
  explicit index_buffer(std::span<const uint32_t> indices);

  void bind() const noexcept;

  index_buffer(const index_buffer&) = delete;
  index_buffer& operator=(const index_buffer&) = delete;

  index_buffer(index_buffer&&) noexcept;
  index_buffer& operator=(index_buffer&&) noexcept;

  ~index_buffer();

 private:
  GLuint id_{0};
};
}  // namespace opengl
