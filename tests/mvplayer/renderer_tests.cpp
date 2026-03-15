extern "C" {
#include <libavutil/pixfmt.h>
}

#include <gtest/gtest.h>

#include "renderer/factory.hpp"

using mvplayer::renderer::factory;

TEST(RendererFactory, SupportsFormatRGB24) {
  EXPECT_TRUE(factory::supports_format(AV_PIX_FMT_RGB24));
}

TEST(RendererFactory, SupportsFormatYUV420P) {
  EXPECT_TRUE(factory::supports_format(AV_PIX_FMT_YUV420P));
}

TEST(RendererFactory, SupportsFormatYUV422P) {
  EXPECT_TRUE(factory::supports_format(AV_PIX_FMT_YUV422P));
}

TEST(RendererFactory, SupportsFormatYUV440P) {
  EXPECT_TRUE(factory::supports_format(AV_PIX_FMT_YUV440P));
}

TEST(RendererFactory, SupportsFormatYUV444P) {
  EXPECT_TRUE(factory::supports_format(AV_PIX_FMT_YUV444P));
}

TEST(RendererFactory, SupportsFormatYUV420P10LE) {
  EXPECT_TRUE(factory::supports_format(AV_PIX_FMT_YUV420P10LE));
}

TEST(RendererFactory, SupportsFormatYUV422P10LE) {
  EXPECT_TRUE(factory::supports_format(AV_PIX_FMT_YUV422P10LE));
}

TEST(RendererFactory, SupportsFormatYUV440P10LE) {
  EXPECT_TRUE(factory::supports_format(AV_PIX_FMT_YUV440P10LE));
}

TEST(RendererFactory, SupportsFormatYUV444P10LE) {
  EXPECT_TRUE(factory::supports_format(AV_PIX_FMT_YUV444P10LE));
}

TEST(RendererFactory, DoesNotSupportNV12) {
  EXPECT_FALSE(factory::supports_format(AV_PIX_FMT_NV12));
}

TEST(RendererFactory, DoesNotSupportNone) {
  EXPECT_FALSE(factory::supports_format(AV_PIX_FMT_NONE));
}

TEST(RendererFactory, DoesNotSupportBGR24) {
  EXPECT_FALSE(factory::supports_format(AV_PIX_FMT_BGR24));
}
