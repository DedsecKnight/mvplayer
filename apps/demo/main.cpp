
#include <spdlog/spdlog.h>

#include <CLI/CLI.hpp>

#include "player.hpp"

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
    spdlog::info("Detected video stream: resolution {} x {}. Bit rate: {}",
                 videoInfo->width, videoInfo->height, videoInfo->bitRate);
    spdlog::info("Video Codec: {}", videoInfo->codecName);

    for (auto frameInfo = player.getFrame(); frameInfo.has_value();
         frameInfo = player.getFrame()) {
      spdlog::info("Received new frame: pts = {}; dts = {}", frameInfo->pts,
                   frameInfo->dts);
    }
  }
}
