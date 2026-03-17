#include <gtest/gtest.h>

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
