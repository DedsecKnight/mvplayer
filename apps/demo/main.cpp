
#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>

#include <CLI/CLI.hpp>
#include <chrono>
#include <opencv2/imgcodecs.hpp>

#include "frame_renderer.hpp"
#include "video_reader.hpp"

int main(int argc, char** argv) {
  CLI::App app;
  std::string input_filename{};
  app.add_option("-i", input_filename, "Input video filename")->required();
  CLI11_PARSE(app, argc, argv);

  mvplayer::video_reader player{};

  if (auto video_info = player.load_video(input_filename);
      video_info.has_value()) {
    spdlog::info(
        std::format("Succesfully read video file. Format {}, duration {} us",
                    video_info->format, video_info->duration));
    spdlog::info(
        "Detected video stream: resolution {} x {} @ {}fps. Bit rate: {}",
        video_info->width, video_info->height,
        video_info->fps.num / video_info->fps.den, video_info->bit_rate);
    spdlog::info("Video Codec: {}", video_info->codec_name);

    mvplayer::frame_renderer renderer{video_info->width, video_info->height,
                                      10};
    SDL_Event e;

    for (auto frame_info = player.get_frame(); frame_info.has_value();
         frame_info = player.get_frame()) {
      while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) {
          return 0;
        }
      }
      auto start = std::chrono::high_resolution_clock::now();
      if (!renderer.render_frame(frame_info->frame)) {
        spdlog::error("Error rendering frame");
        return 1;
      }
      auto end = std::chrono::high_resolution_clock::now();

      auto render_time =
          std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
              .count();
      spdlog::info("Rendered new frame in {}ms: frame = {}; pts = {}; dts = {}",
                   render_time, frame_info->frame_no, frame_info->pts,
                   frame_info->dts);
    }
  }
}
