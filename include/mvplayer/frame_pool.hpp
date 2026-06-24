#pragma once

#include "error.hpp"
#include "utils/owned.hpp"

extern "C" {
#include <libavutil/frame.h>
}

#include <expected>

#include "utils/queue.hpp"
namespace mvplayer {
struct frame_data {
  AVFrame* frame;
  bool requires_unref;
};

class frame_pool {
 public:
  explicit frame_pool(size_t pool_size = 3);

  frame_pool(const frame_pool&) = delete;
  frame_pool& operator=(const frame_pool&) = delete;

  frame_pool(frame_pool&&) = delete;
  frame_pool& operator=(frame_pool&&) = delete;

  ~frame_pool() = default;

  [[nodiscard]] std::expected<AVFrame*, error> get_frame(
      AVCodecContext* codec_ctx_ptr) noexcept;

  void release_frame(AVFrame* frame) noexcept;

 private:
  multithreaded::utils::spsc_queue<frame_data> frame_queue_;
  std::vector<av_frame> frame_owner_;
  std::vector<AVFrame*> free_frames_;
  size_t capacity_;
};
}  // namespace mvplayer
