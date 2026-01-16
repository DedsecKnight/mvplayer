#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
}

namespace mvplayer {

class media_context {
 public:
  media_context() = default;
  media_context(AVCodecParameters* codec_params_ptr, const AVCodec* codec_ptr);

  media_context(const media_context& ctx);
  media_context& operator=(const media_context& ctx);

  media_context(media_context&& ctx) noexcept;
  media_context& operator=(media_context&& ctx) noexcept;

  ~media_context() noexcept;

  [[nodiscard]] const AVCodec& codec() const noexcept { return *codec_ptr_; }
  [[nodiscard]] AVCodecContext& codec_ctx() const noexcept {
    return *codec_ctx_ptr_;
  }

 private:
  [[nodiscard]] static AVCodecContext* initialize_codec_context(
      const media_context& ctx) noexcept;

  AVCodecContext* codec_ctx_ptr_{nullptr};
  const AVCodec* codec_ptr_{nullptr};
};
}  // namespace mvplayer
