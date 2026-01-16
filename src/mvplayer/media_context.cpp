#include "media_context.hpp"

#include <stdexcept>

namespace mvplayer {
media_context::media_context(AVCodecParameters* codec_params_ptr,
                             const AVCodec* codec_ptr)
    : codec_ctx_ptr_{avcodec_alloc_context3(codec_ptr)}, codec_ptr_{codec_ptr} {
  if (avcodec_parameters_to_context(codec_ctx_ptr_, codec_params_ptr) < 0) {
    throw std::runtime_error{"error initializing codec context from params"};
  }
  if (avcodec_open2(codec_ctx_ptr_, codec_ptr_, nullptr) < 0) {
    throw std::runtime_error{"error initializing codec context to use codec"};
  }
}

media_context::media_context(const media_context& ctx)
    : codec_ctx_ptr_{initialize_codec_context(ctx)},
      codec_ptr_{ctx.codec_ptr_} {
  if (codec_ctx_ptr_ == nullptr) {
    throw std::runtime_error{"error copying codec context"};
  }
}

media_context& media_context::operator=(const media_context& ctx) {
  if (this == &ctx) {
    return *this;
  }
  codec_ptr_ = ctx.codec_ptr_;
  codec_ctx_ptr_ = initialize_codec_context(ctx);
  if (codec_ctx_ptr_ == nullptr) {
    throw std::runtime_error{"error copying codec context"};
  }
  return *this;
}

media_context::media_context(media_context&& ctx) noexcept
    : codec_ctx_ptr_{ctx.codec_ctx_ptr_}, codec_ptr_{ctx.codec_ptr_} {
  ctx.codec_ptr_ = nullptr;
  ctx.codec_ctx_ptr_ = nullptr;
}

media_context& media_context::operator=(media_context&& ctx) noexcept {
  codec_ctx_ptr_ = ctx.codec_ctx_ptr_;
  codec_ptr_ = ctx.codec_ptr_;

  ctx.codec_ptr_ = nullptr;
  ctx.codec_ctx_ptr_ = nullptr;

  return *this;
}

media_context::~media_context() noexcept {
  if (codec_ctx_ptr_ != nullptr) {
    avcodec_free_context(&codec_ctx_ptr_);
  }
}

[[nodiscard]] AVCodecContext* media_context::initialize_codec_context(
    const media_context& ctx) const noexcept {
  AVCodecContext* codec_ctx_ptr = avcodec_alloc_context3(ctx.codec_ptr_);
  if (codec_ctx_ptr == nullptr) {
    return nullptr;
  }
  AVCodecParameters ctx_params;
  if (avcodec_parameters_from_context(&ctx_params, ctx.codec_ctx_ptr_) < 0) {
    avcodec_free_context(&codec_ctx_ptr);
    return nullptr;
  }
  if (avcodec_parameters_to_context(codec_ctx_ptr_, &ctx_params) < 0) {
    avcodec_free_context(&codec_ctx_ptr);
    return nullptr;
  }
  if (avcodec_open2(codec_ctx_ptr_, codec_ptr_, nullptr) < 0) {
    avcodec_free_context(&codec_ctx_ptr);
    return nullptr;
  }
  return codec_ctx_ptr;
}
}  // namespace mvplayer
