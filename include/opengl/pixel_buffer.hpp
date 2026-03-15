#pragma once

#include <glad/gl.h>

namespace opengl {
class pixel_buffer {
 public:
  pixel_buffer();

  pixel_buffer(const pixel_buffer&) = delete;
  pixel_buffer& operator=(const pixel_buffer&) = delete;

  pixel_buffer(pixel_buffer&&) noexcept;
  pixel_buffer& operator=(pixel_buffer&&) noexcept;

  ~pixel_buffer();

  void bind() const noexcept;

  void update_pixel(void* pixel_buffer,
                    GLsizeiptr pixel_buffer_size) const noexcept;

  [[nodiscard]] GLuint id() const noexcept;

 private:
  GLuint id_{0};
};
}  // namespace opengl
