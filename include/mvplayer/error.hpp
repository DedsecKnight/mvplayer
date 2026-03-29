#pragma once

#include <fmt/base.h>
#include <fmt/format.h>
#include <glad/gl.h>
extern "C" {
#include <libavutil/error.h>
}

#include <string_view>
#include <variant>
namespace mvplayer {

#define GL_INVOKE(x, ctx)                                                    \
  {                                                                          \
    while (glGetError() != GL_NO_ERROR);                                     \
    x;                                                                       \
    if (auto error = glGetError()) {                                         \
      return std::unexpected(opengl_error{.context = (ctx), .code = error}); \
    }                                                                        \
  }

struct allocation_error {
  std::string_view context;

  auto format(fmt::format_context& ctx) const noexcept {
    return fmt::format_to(ctx.out(), "allocation error at {}", context);
  }
};

struct av_error {
  int32_t code;
  std::string_view context;

  auto format(fmt::format_context& ctx) const noexcept {
    return fmt::format_to(ctx.out(), "averror at {}: {}", context,
                          av_err2str(code));
  }
};

struct mismatch_sample_written_error {
  std::string_view context;
  int32_t expected, actual;

  auto format(fmt::format_context& ctx) const noexcept {
    return fmt::format_to(ctx.out(),
                          "[{}]: expected {} to be written, but {} found",
                          context, expected, actual);
  }
};

struct audio_processing_error {
  std::string_view context;
  std::string msg;

  auto format(fmt::format_context& ctx) const noexcept {
    return fmt::format_to(ctx.out(), "failed to process audio at {}: {}",
                          context, msg);
  }
};

struct video_file_load_error {
  std::string filename;
  std::string err_msg;

  auto format(fmt::format_context& ctx) const noexcept {
    return fmt::format_to(ctx.out(), "failed to read video file {}: {}",
                          filename, err_msg);
  }
};

struct opengl_error {
  std::string_view context;
  GLenum code;
  auto format(fmt::format_context& ctx) const noexcept {
    std::string_view err_msg;
    switch (code) {
      case GL_INVALID_ENUM:
        err_msg = "invalid enum";
        break;
      case GL_INVALID_VALUE:
        err_msg = "invalid value";
        break;
      case GL_INVALID_OPERATION:
        err_msg = "invalid operation";
        break;
      case GL_STACK_OVERFLOW:
        err_msg = "stack overflow";
        break;
      case GL_STACK_UNDERFLOW:
        err_msg = "stack underflow";
        break;
      case GL_OUT_OF_MEMORY:
        err_msg = "out of memory";
        break;
      case GL_INVALID_FRAMEBUFFER_OPERATION:
        err_msg = "invalid framebuffer operation";
        break;
      default:
        err_msg = "unknown error";
        break;
    }
    return fmt::format_to(ctx.out(), "opengl error at {}: {}", context,
                          err_msg);
  }
};

using error =
    std::variant<allocation_error, av_error, mismatch_sample_written_error,
                 audio_processing_error, video_file_load_error, opengl_error>;

}  // namespace mvplayer

template <>
struct fmt::formatter<mvplayer::error> : fmt::formatter<std::string_view> {
  // NOLINTNEXTLINE
  format_context::iterator format(const mvplayer::error& err,
                                  format_context& ctx) const {
    return std::visit([&ctx](const auto& err) { return err.format(ctx); }, err);
  }
};
