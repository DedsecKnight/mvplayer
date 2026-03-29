#include "frame_pool.hpp"

extern "C" {
#include <libavutil/error.h>
}

#include <cerrno>

namespace mvplayer {
frame_pool::frame_pool(size_t pool_size)
    : frame_queue_(pool_size), capacity_(pool_size) {
  spdlog::info("[frame-pool] Initializing a frame pool of size {}", pool_size);
  free_frames_.reserve(pool_size);
  frame_owner_.reserve(pool_size);
  for (size_t i = 0; i < pool_size; ++i) {
    frame_owner_.emplace_back(av_frame_alloc());
    if (auto* frame = frame_owner_.back().get(); frame != nullptr) {
      free_frames_.push_back(frame);
    } else {
      spdlog::critical("[frame-pool] failed to allocate av_frame");
    }
  }
}

[[nodiscard]] std::expected<AVFrame*, error> frame_pool::get_frame(
    AVCodecContext* codec_ctx_ptr) noexcept {
  AVFrame* frame{nullptr};
  bool pop_required = true;

  if (!free_frames_.empty()) {
    frame = free_frames_.back();
  } else if (!frame_queue_.pop(frame)) {
    spdlog::info(
        "[frame-pool] Ran out of frame. Increase capacity from {} to {}",
        capacity_, capacity_ * 2);
    for (size_t i = 0; i < capacity_; ++i) {
      frame_owner_.emplace_back(av_frame_alloc());
      free_frames_.push_back(frame_owner_.back().get());
    }
    capacity_ *= 2;

    frame = free_frames_.back();
  } else {
    pop_required = false;
  }

  if (avcodec_receive_frame(codec_ctx_ptr, frame) == AVERROR(EAGAIN)) {
    if (!pop_required) {
      free_frames_.push_back(frame);
    }
    return std::unexpected(
        av_error{.code = AVERROR(EAGAIN), .context = "avcodec_receive_frame"});
  }

  if (pop_required) {
    free_frames_.pop_back();
  }
  return frame;
}

void frame_pool::release_frame(AVFrame* frame) noexcept {
  av_frame_unref(frame);
  if (!frame_queue_.push(frame)) [[unlikely]] {
    spdlog::critical(
        "[frame-pool] Frame pool buffer queue is full. Trying again...");
    while (!frame_queue_.push(frame)) {
    }
  }
}
}  // namespace mvplayer
