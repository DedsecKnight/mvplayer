#pragma once

#include <fmt/base.h>
#include <fmt/format.h>
extern "C" {
#include <libavutil/error.h>
}

#include <string>
#include <string_view>
#include <variant>
namespace mvplayer {

struct allocation_error {
  std::string context;

  auto format(fmt::format_context& ctx) const noexcept {
    return fmt::format_to(ctx.out(), "ffmpeg allocation error at {}", context);
  }
};

struct av_error {
  int32_t code;
  std::string context;

  auto format(fmt::format_context& ctx) const noexcept {
    return fmt::format_to(ctx.out(), "averror at {}: {}", context,
                          av_err2str(code));
  }
};

using error = std::variant<allocation_error, av_error>;

}  // namespace mvplayer

template <>
struct fmt::formatter<mvplayer::error> : fmt::formatter<std::string_view> {
  // NOLINTNEXTLINE
  format_context::iterator format(const mvplayer::error& err,
                                  format_context& ctx) const {
    return std::visit([&ctx](const auto& err) { return err.format(ctx); }, err);
  }
};
