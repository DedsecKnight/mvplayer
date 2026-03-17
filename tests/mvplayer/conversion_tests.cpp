extern "C" {
#include <libavutil/samplefmt.h>
}

#include <SDL3/SDL_audio.h>
#include <gtest/gtest.h>

#include "utils/conversion.hpp"

using mvplayer::utils::to_sdl_format;

TEST(ToSdlFormat, U8) {
  EXPECT_EQ(to_sdl_format(AV_SAMPLE_FMT_U8), SDL_AUDIO_U8);
}

TEST(ToSdlFormat, S16) {
  EXPECT_EQ(to_sdl_format(AV_SAMPLE_FMT_S16), SDL_AUDIO_S16);
}

TEST(ToSdlFormat, S32) {
  EXPECT_EQ(to_sdl_format(AV_SAMPLE_FMT_S32), SDL_AUDIO_S32);
}

TEST(ToSdlFormat, FLT) {
  EXPECT_EQ(to_sdl_format(AV_SAMPLE_FMT_FLT), SDL_AUDIO_F32);
}

TEST(ToSdlFormat, DBL) {
  EXPECT_EQ(to_sdl_format(AV_SAMPLE_FMT_DBL), SDL_AUDIO_UNKNOWN);
}

TEST(ToSdlFormat, U8P) {
  EXPECT_EQ(to_sdl_format(AV_SAMPLE_FMT_U8P), SDL_AUDIO_U8);
}

TEST(ToSdlFormat, S16P) {
  EXPECT_EQ(to_sdl_format(AV_SAMPLE_FMT_S16P), SDL_AUDIO_S16);
}

TEST(ToSdlFormat, S32P) {
  EXPECT_EQ(to_sdl_format(AV_SAMPLE_FMT_S32P), SDL_AUDIO_S32);
}

TEST(ToSdlFormat, FLTP) {
  EXPECT_EQ(to_sdl_format(AV_SAMPLE_FMT_FLTP), SDL_AUDIO_F32);
}

TEST(ToSdlFormat, DBLP) {
  EXPECT_EQ(to_sdl_format(AV_SAMPLE_FMT_DBLP), SDL_AUDIO_UNKNOWN);
}

TEST(ToSdlFormat, None) {
  EXPECT_EQ(to_sdl_format(AV_SAMPLE_FMT_NONE), SDL_AUDIO_UNKNOWN);
}

TEST(ToSdlFormat, NB) {
  EXPECT_EQ(to_sdl_format(AV_SAMPLE_FMT_NB), SDL_AUDIO_UNKNOWN);
}
