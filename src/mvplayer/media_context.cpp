#include "media_context.hpp"

extern "C" {
#include <libavutil/avutil.h>
#include <libavutil/mathematics.h>
}

#include <atomic>
#include <stdexcept>

namespace mvplayer {
media_context::media_context(AVFormatContext* format_ctx_ptr, AVStream* stream,
                             AVCodecParameters* codec_params_ptr,
                             const AVCodec* codec_ptr)
    : format_ctx_ptr_{format_ctx_ptr},
      codec_ctx_ptr_{avcodec_alloc_context3(codec_ptr)},
      codec_ptr_{codec_ptr},
      stream_{stream} {
  if (avcodec_parameters_to_context(codec_ctx_ptr_, codec_params_ptr) < 0) {
    throw std::runtime_error{"error initializing codec context from params"};
  }
  if (avcodec_open2(codec_ctx_ptr_, codec_ptr_, nullptr) < 0) {
    throw std::runtime_error{"error initializing codec context to use codec"};
  }
}

media_context::media_context(const media_context& ctx)
    : format_ctx_ptr_{ctx.format_ctx_ptr_},
      codec_ctx_ptr_{initialize_codec_context(ctx)},
      codec_ptr_{ctx.codec_ptr_},
      stream_{ctx.stream_} {
  if (codec_ctx_ptr_ == nullptr) {
    throw std::runtime_error{"error copying codec context"};
  }
}

media_context& media_context::operator=(const media_context& ctx) {
  if (this == &ctx) {
    return *this;
  }
  format_ctx_ptr_ = ctx.format_ctx_ptr_;
  codec_ptr_ = ctx.codec_ptr_;
  codec_ctx_ptr_ = initialize_codec_context(ctx);
  stream_ = ctx.stream_;
  if (codec_ctx_ptr_ == nullptr) {
    throw std::runtime_error{"error copying codec context"};
  }
  return *this;
}

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

[[nodiscard]] AVCodecContext* media_context::initialize_codec_context(
    const media_context& ctx) noexcept {
  AVCodecContext* codec_ctx_ptr = avcodec_alloc_context3(ctx.codec_ptr_);
  if (codec_ctx_ptr == nullptr) {
    return nullptr;
  }

  AVCodecParameters* ctx_params_ptr = avcodec_parameters_alloc();
  if (ctx_params_ptr == nullptr) {
    avcodec_free_context(&codec_ctx_ptr);
    return nullptr;
  }

  if (avcodec_parameters_from_context(ctx_params_ptr, ctx.codec_ctx_ptr_) < 0) {
    avcodec_parameters_free(&ctx_params_ptr);
    avcodec_free_context(&codec_ctx_ptr);
    return nullptr;
  }

  if (avcodec_parameters_to_context(codec_ctx_ptr, ctx_params_ptr) < 0) {
    avcodec_parameters_free(&ctx_params_ptr);
    avcodec_free_context(&codec_ctx_ptr);
    return nullptr;
  }

  avcodec_parameters_free(&ctx_params_ptr);

  if (avcodec_open2(codec_ctx_ptr, ctx.codec_ptr_, nullptr) < 0) {
    avcodec_free_context(&codec_ctx_ptr);
    return nullptr;
  }
  return codec_ctx_ptr;
}
}  // namespace mvplayer
