#include "utils/conversion.hpp"

#include <SDL3/SDL_audio.h>

#include <algorithm>

extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libswscale/swscale.h>
}

namespace mvplayer::utils {

SDL_AudioFormat to_sdl_format(AVSampleFormat ff_sample_format) {
  namespace ranges = std::ranges;
  const auto mapping_it = ranges::find_if(
      AUDIO_FORMAT_MAPPING, [ff_sample_format](const auto& audio_mapping) {
        return audio_mapping.first == ff_sample_format;
      });
  return mapping_it->second;
}
}  // namespace mvplayer::utils
