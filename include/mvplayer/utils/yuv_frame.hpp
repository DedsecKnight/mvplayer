#pragma once

#include <array>
#include <cstring>
extern "C" {
#include <libavutil/frame.h>
}

#include <cstdint>
#include <vector>
namespace mvplayer {
struct yuv_frame {
  static constexpr size_t NUM_PLANES = 3;
  std::array<std::vector<uint8_t>, NUM_PLANES> data{};
  std::array<int, NUM_PLANES> linesize{};

  yuv_frame() = default;
  explicit yuv_frame(AVFrame* frame) {
    for (size_t i = 0; i < yuv_frame::NUM_PLANES; ++i) {
      // NOLINTBEGIN
      const auto height = (i == 0 ? frame->height : frame->height / 2);
      data.at(i).resize(frame->linesize[i] * height);
      std::memcpy(data.at(i).data(), frame->data[i], data.at(i).size());
      linesize[i] = frame->linesize[i];
      // NOLINTEND
    }
  }
};
}  // namespace mvplayer
