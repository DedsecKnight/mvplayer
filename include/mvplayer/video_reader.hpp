#pragma once

#include <atomic>
#include <filesystem>
#include <optional>
#include <span>

#include "events.hpp"
#include "events/handler.hpp"
#include "info.hpp"
#include "processor/termination_handler.hpp"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace mvplayer {
class video_reader
    : public multithreaded::events::handlers<events::playback_toggled>,
      public multithreaded::processor::termination_handler {
 private:
  using playback_toggled_event =
      multithreaded::events::envelope<events::playback_toggled>;

 public:
  ~video_reader() noexcept override;

  video_reader() = default;

  video_reader(const video_reader&) = delete;
  video_reader& operator=(const video_reader&) = delete;

  video_reader(video_reader&& reader) noexcept;
  video_reader& operator=(video_reader&& reader) noexcept;

  void handle_termination_signal() noexcept override {
    is_terminated_.store(true, std::memory_order_relaxed);
  }

  [[nodiscard]] std::optional<video_info> load_video(
      const std::filesystem::path& filename) noexcept;
  void operator()(
      [[maybe_unused]] const playback_toggled_event& event) override {
    is_paused_.store(!is_paused_.load(std::memory_order_relaxed),
                     std::memory_order_relaxed);
  }
  void on_startup(std::span<char* const> args) noexcept;

 private:
  [[nodiscard]] std::span<AVStream*> get_media_streams() const noexcept;
  [[nodiscard]] bool load_codec_context(
      const AVCodec* codec_ptr, AVCodecParameters* codec_params_ptr) noexcept;

  void decode_video() noexcept;

  AVFormatContext* format_context_ptr_{nullptr};
  AVCodecContext* codec_context_ptr_{nullptr};

  // Separate thread used for decoding frames
  std::thread frame_decoder_;

  // Atomic flags used to synchronize with frame_decoder_ thread
  std::atomic<bool> is_paused_{false}, is_terminated_{false};
};
};  // namespace mvplayer
