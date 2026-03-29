#include "media_context.hpp"

#include <libavcodec/avcodec.h>

#include "error.hpp"

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/mathematics.h>
}

#include <atomic>

namespace mvplayer {
media_context::media_context(AVFormatContext* format_ctx_ptr, AVStream* stream,
                             AVCodecContext* codec_ctx_ptr,
                             const AVCodec* codec_ptr)
    : format_ctx_ptr_{format_ctx_ptr},
      codec_ctx_ptr_{codec_ctx_ptr},
      codec_ptr_{codec_ptr},
      stream_{stream} {}

media_context::media_context(media_context&& ctx) noexcept {
  std::swap(ctx.format_ctx_ptr_, format_ctx_ptr_);
  std::swap(ctx.codec_ptr_, codec_ptr_);
  std::swap(ctx.codec_ctx_ptr_, codec_ctx_ptr_);
  std::swap(ctx.stream_, stream_);
}

media_context& media_context::operator=(media_context&& ctx) noexcept {
  std::swap(ctx.format_ctx_ptr_, format_ctx_ptr_);
  std::swap(ctx.codec_ptr_, codec_ptr_);
  std::swap(ctx.codec_ctx_ptr_, codec_ctx_ptr_);
  std::swap(ctx.stream_, stream_);

  return *this;
}

media_context::~media_context() noexcept {
  if (codec_ctx_ptr_ != nullptr) {
    avcodec_free_context(&codec_ctx_ptr_);
  }
}

[[nodiscard]] int64_t media_context::most_recent_pts() const noexcept {
  return most_recent_pts_.load(std::memory_order_acquire);
}

void media_context::update_most_recent_pts(int64_t pts) noexcept {
  most_recent_pts_.store(pts, std::memory_order_release);
}

void media_context::flush_codec_context() const noexcept {
  if (codec_ctx_ptr_ == nullptr) {
    return;
  }
  avcodec_flush_buffers(codec_ctx_ptr_);
}

[[nodiscard]] std::expected<media_context, error> media_context::create(
    AVFormatContext* format_ctx_ptr, AVStream* stream,
    AVCodecParameters* codec_params_ptr, const AVCodec* codec_ptr) noexcept {
  auto* codec_ctx_ptr = avcodec_alloc_context3(codec_ptr);

  if (codec_ctx_ptr == nullptr) {
    return std::unexpected(
        allocation_error{.context = "avcodec_alloc_context3"});
  }
  if (auto ret = avcodec_parameters_to_context(codec_ctx_ptr, codec_params_ptr);
      ret < 0) {
    return std::unexpected(
        av_error{.code = ret, .context = "avcodec_parameters_to_context"});
  }
  codec_ctx_ptr->thread_count = 0;

  if (auto ret = avcodec_open2(codec_ctx_ptr, codec_ptr, nullptr); ret < 0) {
    return std::unexpected(av_error{.code = ret, .context = "avcodec_open2"});
  }

  return media_context{format_ctx_ptr, stream, codec_ctx_ptr, codec_ptr};
}
}  // namespace mvplayer
