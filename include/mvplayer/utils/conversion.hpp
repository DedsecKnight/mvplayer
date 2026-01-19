#pragma once

#include <SDL3/SDL_audio.h>

#include <array>

extern "C" {
#include <libavutil/samplefmt.h>
}

#include "utils/owned.hpp"
namespace mvplayer::utils {

static constexpr std::array<std::pair<AVSampleFormat, SDL_AudioFormat>, 12>
    AUDIO_FORMAT_MAPPING = {
        std::make_pair(AVSampleFormat::AV_SAMPLE_FMT_U8,
                       SDL_AudioFormat::SDL_AUDIO_U8),
        std::make_pair(AVSampleFormat::AV_SAMPLE_FMT_S16,
                       SDL_AudioFormat::SDL_AUDIO_S16),
        std::make_pair(AVSampleFormat::AV_SAMPLE_FMT_S32,
                       SDL_AudioFormat::SDL_AUDIO_S32),
        std::make_pair(AVSampleFormat::AV_SAMPLE_FMT_FLT,
                       SDL_AudioFormat::SDL_AUDIO_F32),
        std::make_pair(AVSampleFormat::AV_SAMPLE_FMT_DBL,
                       SDL_AudioFormat::SDL_AUDIO_UNKNOWN),
        std::make_pair(AVSampleFormat::AV_SAMPLE_FMT_U8P,
                       SDL_AudioFormat::SDL_AUDIO_U8),
        std::make_pair(AVSampleFormat::AV_SAMPLE_FMT_S16P,
                       SDL_AudioFormat::SDL_AUDIO_S16),
        std::make_pair(AVSampleFormat::AV_SAMPLE_FMT_S32P,
                       SDL_AudioFormat::SDL_AUDIO_S32),
        std::make_pair(AVSampleFormat::AV_SAMPLE_FMT_FLTP,
                       SDL_AudioFormat::SDL_AUDIO_F32),
        std::make_pair(AVSampleFormat::AV_SAMPLE_FMT_DBLP,
                       SDL_AudioFormat::SDL_AUDIO_UNKNOWN),
        std::make_pair(AVSampleFormat::AV_SAMPLE_FMT_NONE,
                       SDL_AudioFormat::SDL_AUDIO_UNKNOWN),
        std::make_pair(AVSampleFormat::AV_SAMPLE_FMT_NB,
                       SDL_AudioFormat::SDL_AUDIO_UNKNOWN),
};

av_frame convert_frame(AVFrame* src_frame, AVPixelFormat dst_format);
SDL_AudioFormat to_sdl_format(AVSampleFormat ff_sample_format);

}  // namespace mvplayer::utils
