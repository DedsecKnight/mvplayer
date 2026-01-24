#include "audio_context.hpp"

#include <spdlog/spdlog.h>

namespace mvplayer {
std::vector<uint8_t> audio_context::pack_planar_audio_sample(
    AVFrame* audio_frame) const noexcept {
  const auto output_fmt = av_get_packed_sample_fmt(codec_ctx_ptr_->sample_fmt);
  auto swr_ctx = create_swr_context(output_fmt);

  std::vector<uint8_t*> out_data{nullptr};
  [[maybe_unused]] int32_t out_linesize{};
  const int32_t num_channels = codec_ctx_ptr_->ch_layout.nb_channels;
  const auto out_sample_size = av_get_bytes_per_sample(output_fmt);

  auto ret = av_samples_alloc(out_data.data(), &out_linesize, num_channels,
                              audio_frame->nb_samples, output_fmt, 0);
  if (ret < 0) {
    spdlog::error("Error allocating arrays and samples");
    return {};
  }

  int32_t num_samples_written =
      swr_convert(swr_ctx.get(), out_data.data(), audio_frame->nb_samples,
                  static_cast<const uint8_t* const*>(audio_frame->data),
                  audio_frame->nb_samples);

  if (num_samples_written < 0) {
    spdlog::error("Error writing samples");
    return {};
  }

  if (num_samples_written != audio_frame->nb_samples) {
    spdlog::error("Expected to write {} samples. Only {} samples are written",
                  audio_frame->nb_samples, num_samples_written);
    return {};
  }

  size_t audio_buffer_size =
      static_cast<size_t>(out_sample_size) * num_channels * num_samples_written;
  std::vector<uint8_t> audio_buffer(audio_buffer_size);
  std::memcpy(audio_buffer.data(), out_data[0], audio_buffer_size);
  av_freep(static_cast<void*>(out_data.data()));

  return audio_buffer;
}

swr_context audio_context::create_swr_context(
    AVSampleFormat output_fmt) const noexcept {
  swr_context swr_ctx{swr_alloc()};
  if (swr_ctx == nullptr) {
    spdlog::error("Error creating SwrContext");
    return swr_ctx;
  }

  av_opt_set_chlayout(swr_ctx.get(), "in_chlayout", &codec_ctx_ptr_->ch_layout,
                      0);
  av_opt_set_chlayout(swr_ctx.get(), "out_chlayout", &codec_ctx_ptr_->ch_layout,
                      0);

  av_opt_set_int(swr_ctx.get(), "in_sample_rate", codec_ctx_ptr_->sample_rate,
                 0);
  av_opt_set_int(swr_ctx.get(), "out_sample_rate", codec_ctx_ptr_->sample_rate,
                 0);

  av_opt_set_sample_fmt(swr_ctx.get(), "in_sample_fmt",
                        codec_ctx_ptr_->sample_fmt, 0);
  av_opt_set_sample_fmt(swr_ctx.get(), "out_sample_fmt", output_fmt, 0);

  if (swr_init(swr_ctx.get()) < 0) {
    spdlog::error("Error initializing SwrContext");
    return swr_context{nullptr};
  }

  return swr_ctx;
}
}  // namespace mvplayer
