#pragma once

#include "pixel_buffer.hpp"
#include "texture.hpp"
namespace mvplayer::renderer {

struct plane_data_spec {
  GLint internal_format;
  GLenum format, data_type;
  uint32_t texture_slot;
  int32_t width, height;
};

class channel_plane {
 public:
  channel_plane() = default;

  channel_plane(const channel_plane&) = delete;
  channel_plane& operator=(const channel_plane&) = delete;

  channel_plane(channel_plane&&) = default;
  channel_plane& operator=(channel_plane&&) = default;

  ~channel_plane() = default;

  [[nodiscard]] bool load_plane(std::span<uint8_t> plane_data,
                                const plane_data_spec& spec) noexcept;

 private:
  std::array<opengl::pixel_buffer, 2> pixel_buffer_;
  opengl::texture texture_;
  size_t pb_index_{0};
  bool initialized_{false};
};
};  // namespace mvplayer::renderer
