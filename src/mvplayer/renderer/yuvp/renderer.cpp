#include "renderer/yuvp/renderer.hpp"

#include <glad/gl.h>
#include <libavutil/pixfmt.h>
#include <spdlog/spdlog.h>

#include "renderer/yuvp/shader_spec.hpp"

namespace mvplayer::renderer::yuvp {
renderer::renderer()
    : base(vertex_shader_spec{}, fragment_shader_spec{}), planes_{{}} {}

bool renderer::pix_fmt_is_10bit(AVPixelFormat pixel_format) noexcept {
  return pixel_format == AV_PIX_FMT_YUV420P10LE ||
         pixel_format == AV_PIX_FMT_YUV444P10LE ||
         pixel_format == AV_PIX_FMT_YUV422P10LE ||
         pixel_format == AV_PIX_FMT_YUV440P10LE;
}

[[nodiscard]] std::expected<void, error> renderer::render_frame(
    const AVFrame* frame) noexcept {
  bool half_width = frame->format == AV_PIX_FMT_YUV420P ||
                    frame->format == AV_PIX_FMT_YUV422P ||
                    frame->format == AV_PIX_FMT_YUV422P10LE ||
                    frame->format == AV_PIX_FMT_YUV420P10LE;
  bool half_height = frame->format == AV_PIX_FMT_YUV420P ||
                     frame->format == AV_PIX_FMT_YUV420P10LE ||
                     frame->format == AV_PIX_FMT_YUV440P ||
                     frame->format == AV_PIX_FMT_YUV440P10LE;

  std::array<int32_t, 3> plane_width{
      frame->width, half_width ? frame->width / 2 : frame->width,
      half_width ? frame->width / 2 : frame->width};
  std::array<int32_t, 3> plane_height{
      frame->height, half_height ? frame->height / 2 : frame->height,
      half_height ? frame->height / 2 : frame->height};
  std::span<uint8_t* const> plane_view{frame->data, 3UZ};        // NOLINT
  std::span<const int32_t> linesize_view{frame->linesize, 3UZ};  // NOLINT

  bool is_10_bit = pix_fmt_is_10bit(static_cast<AVPixelFormat>(frame->format));
  const auto multiplier = is_10_bit ? 64.0F : 1.0F;
  GL_INVOKE(glProgramUniform1f(shader_.id(),
                               glGetUniformLocation(shader_.id(), "multiplier"),
                               multiplier),
            "set multiplier uniform value for yuvp");

  for (auto i{3UZ}; i-- > 0;) {
    std::span<uint8_t> plane_data_view{
        plane_view[i],
        static_cast<size_t>(plane_height.at(i) * linesize_view[i])};
    const auto pixel_value_size =
        is_10_bit ? sizeof(uint16_t) : sizeof(uint8_t);
    const auto unpack_row_length =
        static_cast<int32_t>(linesize_view[i] / pixel_value_size);

    auto res = update_plane_data(
        i, plane_data_view, unpack_row_length,

        {.internal_format = is_10_bit ? GL_R16 : GL_RED,
         .format = GL_RED,
         .data_type = static_cast<GLenum>(is_10_bit ? GL_UNSIGNED_SHORT
                                                    : GL_UNSIGNED_BYTE),
         .texture_slot = static_cast<uint32_t>(i),
         .width = plane_width.at(i),
         .height = plane_height.at(i)});
    if (!res.has_value()) {
      return std::unexpected(res.error());
    }
  }
  draw_frame();
  return {};
}

std::expected<void, error> renderer::update_plane_data(
    size_t plane_index, std::span<uint8_t> plane_data,
    int32_t unpack_row_length, const plane_data_spec& spec) noexcept {
  GL_INVOKE(glPixelStorei(GL_UNPACK_ROW_LENGTH, unpack_row_length),
            "set GL_UNPACK_ROW_LENGTH for yuvp");
  if (auto res = planes_.at(plane_index).load_plane(plane_data, spec);
      !res.has_value()) {
    GL_INVOKE(glPixelStorei(GL_UNPACK_ROW_LENGTH, 0),
              "reset GL_UNPACK_ROW_LENGTH for yuvp");
    return std::unexpected(res.error());
  }
  GL_INVOKE(glPixelStorei(GL_UNPACK_ROW_LENGTH, 0),
            "reset GL_UNPACK_ROW_LENGTH for yuvp");
  return {};
}
}  // namespace mvplayer::renderer::yuvp
