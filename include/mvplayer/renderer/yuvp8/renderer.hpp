#pragma once

#include "renderer/base.hpp"
#include "texture.hpp"
namespace mvplayer::renderer::yuvp8 {
class renderer : public base {
 public:
  renderer();

  renderer(const renderer&) = delete;
  renderer& operator=(const renderer&) = delete;

  renderer(renderer&&) noexcept = default;
  renderer& operator=(renderer&&) noexcept = default;

  ~renderer() override = default;

  [[nodiscard]] bool render_frame(const AVFrame* frame) noexcept override;

 private:
  std::array<opengl::texture, 3> plane_textures_;
};
}  // namespace mvplayer::renderer::yuvp8
