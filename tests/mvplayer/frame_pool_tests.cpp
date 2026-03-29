#include <gtest/gtest.h>

#include "error.hpp"
#include "frame_pool.hpp"

using mvplayer::frame_pool;

TEST(FramePool, DefaultConstruction) {
  frame_pool pool;
}

TEST(FramePool, CustomSize) {
  frame_pool pool(7);
}

TEST(FramePool, MinimumSize) {
  frame_pool pool(1);
}

TEST(FramePool, GetFrameReturnType) {
  frame_pool pool;
  // Verify the return type is std::expected<AVFrame*, error>
  using result_t = decltype(pool.get_frame(nullptr));
  static_assert(
      std::is_same_v<result_t, std::expected<AVFrame*, mvplayer::error>>);
}

TEST(FramePool, ErrorVariantHoldsAvError) {
  // Verify that an av_error can be stored in the error variant
  // and extracted via std::get_if — this is the pattern used in
  // video_reader::decode_video to check for EAGAIN
  mvplayer::error err = mvplayer::av_error{
      .code = AVERROR(EAGAIN), .context = "avcodec_receive_frame"};
  auto* av_err = std::get_if<mvplayer::av_error>(&err);
  ASSERT_NE(av_err, nullptr);
  EXPECT_EQ(av_err->code, AVERROR(EAGAIN));
  EXPECT_EQ(av_err->context, "avcodec_receive_frame");
}
