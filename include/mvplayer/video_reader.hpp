#pragma once

#include <array>
#include <atomic>
#include <filesystem>
#include <optional>
#include <span>

#include "events.hpp"
#include "events/handler.hpp"
#include "frame_pool.hpp"
#include "info.hpp"
#include "media_context.hpp"
#include "processor/termination_handler.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace mvplayer {
class video_reader
    : public multithreaded::events::handlers<events::playback_toggled,
                                             events::seek_request>,
      public multithreaded::processor::termination_handler {
 private:
  using playback_toggled_event =
      multithreaded::events::envelope<events::playback_toggled>;
  using seek_request_event =
      multithreaded::events::envelope<events::seek_request>;

  enum class seek_result : uint8_t {
    no_seek_request_found,
    seek_request_processed,
    seek_error
  };

  static constexpr int64_t seek_unit_secs = 5;

 public:
  ~video_reader() noexcept override;

  video_reader(const std::reference_wrapper<frame_pool>& picture_frame_pool,
               const std::reference_wrapper<frame_pool>& audio_frame_pool)
      : frame_pools_{picture_frame_pool, audio_frame_pool} {}

  video_reader(const video_reader&) = delete;
  video_reader& operator=(const video_reader&) = delete;

  video_reader(video_reader&& reader) noexcept;
  video_reader& operator=(video_reader&& reader) noexcept;

  void handle_termination_signal() noexcept override;

  [[nodiscard]] std::optional<video_info> load_video(
      const std::filesystem::path& filename) noexcept;
  void operator()(
      [[maybe_unused]] const playback_toggled_event& event) override {
    is_paused_.store(!is_paused_.load(std::memory_order_relaxed),
                     std::memory_order_release);
  }
  void operator()(const seek_request_event& event) override;
  void on_startup(std::span<char* const> args) noexcept;

 private:
  [[nodiscard]] std::span<AVStream*> get_media_streams() const noexcept;
  [[nodiscard]] int64_t next_seek_request() noexcept;
  [[nodiscard]] seek_result process_seek_request() noexcept;

  void picture_frame_handler(AVFrame* frame,
                             bool reset_frame_sequence) noexcept;
  void audio_frame_handler(AVFrame* audio_frame,
                           bool reset_frame_sequence) noexcept;

  void decode_video() noexcept;

  AVFormatContext* format_context_ptr_{nullptr};
  media_context frame_ctx_, audio_ctx_;

  // Separate thread used for decoding frames
  std::thread frame_decoder_;

  // Frame pools. frame_pools_[0] is for picture frame, frame_pools_[1] is for
  // audio frame
  std::array<std::reference_wrapper<frame_pool>, 2> frame_pools_;

  // Main thread will write seek request (in pts), decode thread will pull and
  // call av_seek_frame
  std::atomic<int64_t> seek_request_{-1};

  // Atomic flags used to synchronize with frame_decoder_ thread
  std::atomic<bool> is_paused_{false}, is_terminated_{false};
};
};  // namespace mvplayer
