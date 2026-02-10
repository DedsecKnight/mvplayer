#include "audio_renderer.hpp"
#include "engine/engine.hpp"
#include "events.hpp"
#include "frame_renderer.hpp"
#include "video_reader.hpp"

int main(int argc, char** argv) {
  multithreaded::engine engine{};
  constexpr int32_t padding = 10;

  constexpr size_t picture_frame_queue_size = 7;
  constexpr size_t audio_frame_queue_size = 31;

  mvplayer::frame_pool picture_frame_pool{15};
  mvplayer::frame_pool audio_frame_pool{63};

  auto video_reader = engine.create_processor<mvplayer::video_reader>(
      "video-reader", std::ref(picture_frame_pool), std::ref(audio_frame_pool));

  auto frame_renderer = engine.create_processor<mvplayer::frame_renderer>(
      "frame-renderer", padding, std::ref(picture_frame_pool));

  auto audio_renderer = engine.create_processor<mvplayer::audio_renderer>(
      "audio-renderer", std::ref(audio_frame_pool));

  frame_renderer.subscribe_to<mvplayer::events::new_frame_loaded,
                              mvplayer::events::new_video_loaded>(
      video_reader, picture_frame_queue_size);
  video_reader.subscribe_to<mvplayer::events::playback_toggled,
                            mvplayer::events::seek_request>(frame_renderer);

  audio_renderer.subscribe_to<mvplayer::events::new_audio_samples_loaded,
                              mvplayer::events::new_video_loaded>(
      video_reader, audio_frame_queue_size);

  audio_renderer.subscribe_to<mvplayer::events::playback_toggled>(
      frame_renderer);

  engine.start(std::span(argv, argc));
}
