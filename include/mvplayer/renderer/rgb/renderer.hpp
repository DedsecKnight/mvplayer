#pragma once

#include "renderer/base.hpp"
#include "renderer/channel_plane.hpp"

namespace mvplayer::renderer::rgb {
class renderer : public base {
 public:
  renderer();

  renderer(const renderer&) = delete;
  renderer& operator=(const renderer&) = delete;

  renderer(renderer&&) noexcept = default;
  renderer& operator=(renderer&&) noexcept = default;

  [[nodiscard]] std::expected<void, error> render_frame(
      const AVFrame* frame) noexcept override;

  ~renderer() override = default;

 private:
  channel_plane plane_;
};
}  // namespace mvplayer::renderer::rgb
