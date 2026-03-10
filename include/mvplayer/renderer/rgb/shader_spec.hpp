#pragma once

#include <glad/gl.h>

namespace mvplayer::renderer::rgb {
struct vertex_shader_spec {
  static constexpr const int32_t type = GL_VERTEX_SHADER;
  static constexpr const char* source = R"(
    #version 430 core
    layout (location = 0) in vec3 v_pos;
    layout (location = 1) in vec2 tex_pos;

    out vec2 tex_coord;

    void main() {
      gl_Position = vec4(v_pos, 1.0);
      tex_coord = tex_pos;
    }
  )";
};

struct fragment_shader_spec {
  static constexpr int32_t type = GL_FRAGMENT_SHADER;
  static constexpr const char* source = R"(
    #version 430 core
    out vec4 pixel_color;
  
    in vec2 tex_coord;

    uniform sampler2D image_texture;

    void main() {
      pixel_color = texture(image_texture, tex_coord);
    }
  )";
};
}  // namespace mvplayer::renderer::rgb
