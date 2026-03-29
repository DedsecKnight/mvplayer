#pragma once

#include <fmt/base.h>
#include <fmt/format.h>
extern "C" {
#include <libavutil/error.h>
}

#include <string_view>
#include <variant>
namespace mvplayer {

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

using error =
    std::variant<allocation_error, av_error, mismatch_sample_written_error,
                 audio_processing_error, video_file_load_error>;

}  // namespace mvplayer

template <>
struct fmt::formatter<mvplayer::error> : fmt::formatter<std::string_view> {
  // NOLINTNEXTLINE
  format_context::iterator format(const mvplayer::error& err,
                                  format_context& ctx) const {
    return std::visit([&ctx](const auto& err) { return err.format(ctx); }, err);
  }
};
