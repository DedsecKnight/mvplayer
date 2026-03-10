#pragma once

#include "renderer/base.hpp"
#include "texture.hpp"

namespace mvplayer::renderer::rgb {
class renderer : public base {
 public:
  renderer();

  renderer(const renderer&) = delete;
  renderer& operator=(const renderer&) = delete;

  renderer(renderer&&) noexcept = default;
  renderer& operator=(renderer&&) noexcept = default;

  [[nodiscard]] bool render_frame(const AVFrame* frame) noexcept override;

  ~renderer() override = default;

 private:
  opengl::texture texture_;
};
}  // namespace mvplayer::renderer::rgb
