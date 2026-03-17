#include <gtest/gtest.h>

#include "events.hpp"
#include "info.hpp"

using namespace mvplayer;

TEST(SeekDirection, EnumValues) {
  EXPECT_EQ(static_cast<uint8_t>(seek_direction::forward), 0);
  EXPECT_EQ(static_cast<uint8_t>(seek_direction::backward), 1);
}

TEST(PictureInfo, AggregateInit) {
  picture_info info{{"yuv420p", {1, 90000}}, {30, 1}, 1920, 1080};
  EXPECT_EQ(info.format, "yuv420p");
  EXPECT_EQ(info.tbn.num, 1);
  EXPECT_EQ(info.tbn.den, 90000);
  EXPECT_EQ(info.fps.num, 30);
  EXPECT_EQ(info.fps.den, 1);
  EXPECT_EQ(info.width, 1920);
  EXPECT_EQ(info.height, 1080);
}

TEST(AudioInfo, AggregateInit) {
  audio_info info{
      {"fltp", {1, 48000}}, nullptr, AV_SAMPLE_FMT_FLTP, 2, 48000, true};
  EXPECT_EQ(info.format, "fltp");
  EXPECT_EQ(info.sample_format, AV_SAMPLE_FMT_FLTP);
  EXPECT_EQ(info.num_channels, 2);
  EXPECT_EQ(info.frequency, 48000);
  EXPECT_TRUE(info.has_audio_stream);
}

TEST(VideoInfo, NestedStructAccess) {
  video_info info{
      .codec_name = "h264",
      .picture = {{"yuv420p", {1, 90000}}, {24, 1}, 3840, 2160},
      .audio = {{"fltp", {1, 44100}}, nullptr, AV_SAMPLE_FMT_FLTP, 2, 44100,
                true},
  };
  EXPECT_EQ(info.codec_name, "h264");
  EXPECT_EQ(info.picture.width, 3840);
  EXPECT_EQ(info.audio.frequency, 44100);
}

TEST(NewFrameLoaded, Fields) {
  events::new_frame_loaded evt{
      .frame = nullptr,
      .frame_num = 100,
      .frame_pts = 5000,
      .reset_frame_sequence = true,
  };
  EXPECT_EQ(evt.frame, nullptr);
  EXPECT_EQ(evt.frame_num, 100);
  EXPECT_EQ(evt.frame_pts, 5000);
  EXPECT_TRUE(evt.reset_frame_sequence);
}

TEST(SeekRequest, Fields) {
  events::seek_request evt{.direction = seek_direction::backward};
  EXPECT_EQ(evt.direction, seek_direction::backward);
}
