#pragma once

#include <atomic>

#include "info.hpp"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace mvplayer {

class media_context {
 private:
  static constexpr int64_t seek_unit_secs = 5;

 public:
  media_context() = default;
  media_context(AVFormatContext* format_ctx_ptr, AVStream* stream,
                AVCodecParameters* codec_params_ptr, const AVCodec* codec_ptr);

  media_context(const media_context& ctx);
  media_context& operator=(const media_context& ctx);

  media_context(media_context&& ctx) noexcept;
  media_context& operator=(media_context&& ctx) noexcept;

  ~media_context() noexcept;

  [[nodiscard]] const AVCodec& codec() const noexcept { return *codec_ptr_; }
  [[nodiscard]] AVCodecContext& codec_ctx() const noexcept {
    return *codec_ctx_ptr_;
  }

  [[nodiscard]] bool process_seek_request() noexcept;
  void handle_seek_request(seek_direction direction) noexcept;
  void update_most_recent_pts(int64_t pts) noexcept;

 protected:
  [[nodiscard]] static AVCodecContext* initialize_codec_context(
      const media_context& ctx) noexcept;

  AVFormatContext* format_ctx_ptr_{nullptr};
  AVCodecContext* codec_ctx_ptr_{nullptr};
  const AVCodec* codec_ptr_{nullptr};
  const AVStream* stream_{nullptr};
  std::atomic<int64_t> most_recent_pts_{-1};

 private:
  [[nodiscard]] int64_t next_seek_request() noexcept;

  // Main thread will write seek request (in pts), decode thread will pull and
  // call av_seek_frame
  std::atomic<int64_t> seek_request_{-1};
};
}  // namespace mvplayer
