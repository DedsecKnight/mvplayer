#pragma once

#include "error.hpp"
#include "index_buffer.hpp"
#include "shader.hpp"
#include "vertex_array.hpp"
#include "vertex_buffer.hpp"
extern "C" {
#include <libavutil/frame.h>
}

namespace mvplayer::renderer {
class base {
 private:
  static constexpr std::array<float, 20> SCREEN_VERTICES_DATA{
      -1.0F, -1.0F, 0.0F, 0.0F, 0.0F, 1.0F,  -1.0F, 0.0F, 1.0F, 0.0F,
      1.0F,  1.0F,  0.0F, 1.0F, 1.0F, -1.0F, 1.0F,  0.0F, 0.0F, 1.0F};
  static const std::array<opengl::vertex_attribute_spec, 2>
      SCREEN_VERTICES_ATTRIBUTES;
  static constexpr std::array<uint32_t, 6> SCREEN_VERTEX_ORDER{0, 1, 2,
                                                               0, 2, 3};

 public:
  template <opengl::shader_spec_concept vertex_shader_spec,
            opengl::shader_spec_concept fragment_shader_spec>
  base(const vertex_shader_spec& vertex_shader_info,
       const fragment_shader_spec& fragment_shader_info)
      : vbo_{SCREEN_VERTICES_DATA},
        vao_{SCREEN_VERTICES_ATTRIBUTES},
        ibo_{SCREEN_VERTEX_ORDER},
        shader_{vertex_shader_info, fragment_shader_info} {}

  base(const base&) = delete;
  base& operator=(const base&) = delete;

  base(base&&) noexcept = default;
  base& operator=(base&&) noexcept = default;

  [[nodiscard]] virtual std::expected<void, error> render_frame(
      const AVFrame* frame) noexcept = 0;

  virtual ~base() = default;
  void load_shaders() const noexcept;

 protected:
  void draw_frame() const noexcept;

  opengl::vertex_buffer vbo_;
  opengl::vertex_array vao_;
  opengl::index_buffer ibo_;
  opengl::shader shader_;
};
};  // namespace mvplayer::renderer
