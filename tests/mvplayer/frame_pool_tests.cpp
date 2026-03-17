#include <gtest/gtest.h>

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
