#include <gtest/gtest.h>

#include "error.hpp"
#include "media_context.hpp"

using mvplayer::media_context;

TEST(MediaContext, DefaultConstructionPts) {
  media_context ctx;
  EXPECT_EQ(ctx.most_recent_pts(), -1);
}

TEST(MediaContext, UpdatePts) {
  media_context ctx;
  ctx.update_most_recent_pts(42);
  EXPECT_EQ(ctx.most_recent_pts(), 42);
}

TEST(MediaContext, MultiplePtsUpdates) {
  media_context ctx;
  ctx.update_most_recent_pts(10);
  EXPECT_EQ(ctx.most_recent_pts(), 10);
  ctx.update_most_recent_pts(500);
  EXPECT_EQ(ctx.most_recent_pts(), 500);
  ctx.update_most_recent_pts(0);
  EXPECT_EQ(ctx.most_recent_pts(), 0);
}

TEST(MediaContext, FlushNullContext) {
  media_context ctx;
  ctx.flush_codec_context();
}

TEST(MediaContext, MoveConstruction) {
  media_context src;
  src.update_most_recent_pts(99);
  media_context dst(std::move(src));
  // Pointers are swapped but atomic PTS is not (atomics have deleted move)
  // src retains its PTS value, dst gets the default -1
  EXPECT_EQ(dst.most_recent_pts(), -1);
  EXPECT_EQ(src.most_recent_pts(), 99);
}

TEST(MediaContext, MoveAssignment) {
  media_context src;
  src.update_most_recent_pts(77);
  media_context dst;
  dst = std::move(src);
  // Same as move construction: atomic members are not swapped
  EXPECT_EQ(dst.most_recent_pts(), -1);
  EXPECT_EQ(src.most_recent_pts(), 77);
}

TEST(MediaContext, DestructorNullContext) {
  { media_context ctx; }
}

TEST(MediaContext, CreateReturnsExpectedType) {
  // Verify create() returns std::expected<media_context, error>
  using result_t =
      decltype(media_context::create(nullptr, nullptr, nullptr, nullptr));
  static_assert(
      std::is_same_v<result_t,
                     std::expected<media_context, mvplayer::error>>);
}

TEST(MediaContext, CreateWithInvalidCodecParamsReturnsError) {
  // avcodec_alloc_context3(nullptr) succeeds but avcodec_parameters_to_context
  // with empty params fails with an error code
  AVCodecParameters* params = avcodec_parameters_alloc();
  auto result = media_context::create(nullptr, nullptr, params, nullptr);
  avcodec_parameters_free(&params);
  ASSERT_FALSE(result.has_value());
  // The error should be either an av_error (from avcodec_open2 failing)
  // or allocation_error — we just verify it propagates correctly
  EXPECT_TRUE(std::holds_alternative<mvplayer::av_error>(result.error()) ||
              std::holds_alternative<mvplayer::allocation_error>(result.error()));
}
