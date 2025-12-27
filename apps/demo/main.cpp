
#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>

#include <CLI/CLI.hpp>
#include <chrono>
#include <opencv2/imgcodecs.hpp>

#include "player.hpp"
#include "renderer.hpp"

int main(int argc, char** argv) {
  CLI::App app;
  std::string inputFileName{};
  app.add_option("-i", inputFileName, "Input video filename")->required();
  CLI11_PARSE(app, argc, argv);

  mvplayer::VideoPlayer player{};

  if (auto videoInfo = player.loadVideo(inputFileName); videoInfo.has_value()) {
    spdlog::info(
        std::format("Succesfully read video file. Format {}, duration {} us",
                    videoInfo->format, videoInfo->duration));
    spdlog::info(
        "Detected video stream: resolution {} x {} @ {}fps. Bit rate: {}",
        videoInfo->width, videoInfo->height,
        videoInfo->fps.num / videoInfo->fps.den, videoInfo->bitRate);
    spdlog::info("Video Codec: {}", videoInfo->codecName);

    mvplayer::Renderer renderer{videoInfo->width, videoInfo->height, 10};
    SDL_Event e;

    for (auto frameInfo = player.getFrame(); frameInfo.has_value();
         frameInfo = player.getFrame()) {
      while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) {
          return 0;
        }
      }
      auto start = std::chrono::high_resolution_clock::now();
      if (!renderer.renderFrame(frameInfo->frame)) {
        spdlog::error("Error rendering frame");
        return 1;
      }
      auto end = std::chrono::high_resolution_clock::now();

      auto renderTime =
          std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
              .count();
      spdlog::info("Rendered new frame in {}ms: frame = {}; pts = {}; dts = {}",
                   renderTime, frameInfo->frameNo, frameInfo->pts,
                   frameInfo->dts);
    }
  }
}
