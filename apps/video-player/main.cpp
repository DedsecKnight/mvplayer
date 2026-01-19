#include "audio_renderer.hpp"
#include "engine/engine.hpp"
#include "events.hpp"
#include "frame_renderer.hpp"
#include "video_reader.hpp"

int main(int argc, char** argv) {
  multithreaded::engine engine{};
  constexpr int32_t padding = 10;

  auto video_reader =
      engine.create_processor<mvplayer::video_reader>("video-reader");

  auto frame_renderer = engine.create_processor<mvplayer::frame_renderer>(
      "frame-renderer", padding);

  auto audio_renderer =
      engine.create_processor<mvplayer::audio_renderer>("audio-renderer");

  frame_renderer.subscribe_to<mvplayer::events::new_frame_loaded,
                              mvplayer::events::new_video_loaded>(video_reader);
  video_reader.subscribe_to<mvplayer::events::playback_toggled>(frame_renderer);

  audio_renderer.subscribe_to<mvplayer::events::new_audio_samples_loaded,
                              mvplayer::events::new_video_loaded>(video_reader);

  audio_renderer.subscribe_to<mvplayer::events::playback_toggled>(
      frame_renderer);

  engine.start(std::span(argv, argc));
}
