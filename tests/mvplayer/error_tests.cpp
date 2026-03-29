#include <fmt/format.h>
#include <gtest/gtest.h>

#include "error.hpp"

extern "C" {
#include <libavutil/error.h>
}

using namespace mvplayer;

TEST(AllocationError, Format) {
  allocation_error err{.context = "avcodec_alloc_context3"};
  error variant_err = err;
  auto formatted = fmt::format("{}", variant_err);
  EXPECT_NE(formatted.find("avcodec_alloc_context3"), std::string::npos);
  EXPECT_NE(formatted.find("allocation error"), std::string::npos);
}

TEST(AvError, Format) {
  av_error err{.code = AVERROR(ENOMEM), .context = "avcodec_open2"};
  error variant_err = err;
  auto formatted = fmt::format("{}", variant_err);
  EXPECT_NE(formatted.find("avcodec_open2"), std::string::npos);
  EXPECT_NE(formatted.find("averror"), std::string::npos);
}

TEST(AvError, FormatContainsAvStrerror) {
  av_error err{.code = AVERROR(ENOMEM), .context = "test_context"};
  error variant_err = err;
  auto formatted = fmt::format("{}", variant_err);

  char expected_msg[AV_ERROR_MAX_STRING_SIZE]{};
  av_strerror(AVERROR(ENOMEM), expected_msg, sizeof(expected_msg));
  EXPECT_NE(formatted.find(expected_msg), std::string::npos);
}

TEST(MismatchSampleWrittenError, Format) {
  mismatch_sample_written_error err{
      .context = "swr_convert", .expected = 1024, .actual = 512};
  error variant_err = err;
  auto formatted = fmt::format("{}", variant_err);
  EXPECT_NE(formatted.find("swr_convert"), std::string::npos);
  EXPECT_NE(formatted.find("1024"), std::string::npos);
  EXPECT_NE(formatted.find("512"), std::string::npos);
}

TEST(AudioProcessingError, Format) {
  audio_processing_error err{.context = "SDL_PutAudioStreamData",
                             .msg = "device not found"};
  error variant_err = err;
  auto formatted = fmt::format("{}", variant_err);
  EXPECT_NE(formatted.find("SDL_PutAudioStreamData"), std::string::npos);
  EXPECT_NE(formatted.find("device not found"), std::string::npos);
}

TEST(VideoFileLoadError, Format) {
  video_file_load_error err{.filename = "test.mp4",
                            .err_msg = "file not found"};
  error variant_err = err;
  auto formatted = fmt::format("{}", variant_err);
  EXPECT_NE(formatted.find("test.mp4"), std::string::npos);
  EXPECT_NE(formatted.find("file not found"), std::string::npos);
}

TEST(ErrorVariant, DispatchesToCorrectFormatter) {
  error alloc_err = allocation_error{.context = "alloc_test"};
  error av_err = av_error{.code = AVERROR(EAGAIN), .context = "av_test"};
  error mismatch_err = mismatch_sample_written_error{
      .context = "mismatch_test", .expected = 10, .actual = 5};
  error audio_err =
      audio_processing_error{.context = "audio_test", .msg = "fail"};
  error video_err =
      video_file_load_error{.filename = "video_test.mp4", .err_msg = "fail"};

  EXPECT_NE(fmt::format("{}", alloc_err).find("allocation error"),
            std::string::npos);
  EXPECT_NE(fmt::format("{}", av_err).find("averror"), std::string::npos);
  EXPECT_NE(fmt::format("{}", mismatch_err).find("expected"),
            std::string::npos);
  EXPECT_NE(fmt::format("{}", audio_err).find("process audio"),
            std::string::npos);
  EXPECT_NE(fmt::format("{}", video_err).find("video file"),
            std::string::npos);
}
