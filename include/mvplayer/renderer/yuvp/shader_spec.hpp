#pragma once

#include <glad/gl.h>
namespace mvplayer::renderer::yuvp {
struct vertex_shader_spec {
  static constexpr const int32_t type = GL_VERTEX_SHADER;
  static constexpr const char* source = R"(
    #version 430 core
    layout (location = 0) in vec3 v_pos;
    layout (location = 1) in vec2 tex_pos;

    out vec2 tex_coord;

    void main() {
      gl_Position = vec4(v_pos, 1.0);
      tex_coord = vec2(tex_pos.x, 1.0 - tex_pos.y);
    }
  )";
};

struct fragment_shader_spec {
  static constexpr int32_t type = GL_FRAGMENT_SHADER;
  static constexpr const char* source = R"(
    #version 430 core
    out vec4 pixel_color;

    in vec2 tex_coord;

    layout(binding = 0) uniform sampler2D y_texture;
    layout(binding = 1) uniform sampler2D u_texture;
    layout(binding = 2) uniform sampler2D v_texture;
    uniform float multiplier;

    void main() {
      float y_pixel = texture(y_texture, tex_coord).r * multiplier;
      float u_pixel = texture(u_texture, tex_coord).r * multiplier - 0.5;
      float v_pixel = texture(v_texture, tex_coord).r * multiplier - 0.5;

      float r_pixel = y_pixel + 1.402    * v_pixel;
      float g_pixel = y_pixel - 0.344136 * u_pixel - 0.714136 * v_pixel;
      float b_pixel = y_pixel + 1.772    * u_pixel;

      pixel_color = vec4(r_pixel, g_pixel, b_pixel, 1.0);
    }
  )";
};
}  // namespace mvplayer::renderer::yuvp
