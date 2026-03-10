#include "renderer/base.hpp"

#include "vertex_array.hpp"

namespace mvplayer::renderer {
const std::array<opengl::vertex_attribute_spec, 2>
    base::SCREEN_VERTICES_ATTRIBUTES = {
        opengl::vertex_attribute_spec{.component_size = 3,
                                      .data_type = GL_FLOAT,
                                      .normalized = GL_FALSE,
                                      .stride = 5 * sizeof(float),
                                      .offset = nullptr},
        opengl::vertex_attribute_spec{
            .component_size = 2,
            .data_type = GL_FLOAT,
            .normalized = GL_FALSE,
            .stride = 5 * sizeof(float),
            .offset = reinterpret_cast<void*>(3 * sizeof(float))}};  // NOLINT

void base::draw_frame() const noexcept {
  glClearColor(0.0F, 0.0F, 0.0F, 1.0F);  // NOLINT
  glClear(GL_COLOR_BUFFER_BIT);
  vao_.bind();
  shader_.use();
  glDrawElements(GL_TRIANGLES, static_cast<int32_t>(SCREEN_VERTEX_ORDER.size()),
                 GL_UNSIGNED_INT, nullptr);
}

void base::load_shaders() const noexcept { shader_.use(); }
}  // namespace mvplayer::renderer
