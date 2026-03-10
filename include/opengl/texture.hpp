#pragma once

#include <glad/gl.h>

#include <span>
namespace opengl {

struct texture_spec {
  int32_t width, height;
};

class texture {
 public:
  texture();
  texture(std::span<uint8_t> pixel_buffer, GLint pixel_format,
          const texture_spec& spec);

  void configure_texture_data(std::span<uint8_t> pixel_buffer,
                              GLint pixel_format,
                              const texture_spec& spec) noexcept;
  void bind(uint32_t slot = 0) const noexcept;

  texture(const texture&) = delete;
  texture& operator=(const texture&) = delete;

  texture(texture&&) noexcept;
  texture& operator=(texture&&) noexcept;

  ~texture();

 private:
  void configure_filtering() const noexcept;

  GLuint id_{0};
  bool initialized_{false};
};
}  // namespace opengl
