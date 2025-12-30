#pragma once

#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
}

#include <SDL3/SDL.h>

namespace mvplayer {

namespace deallocator {
void AVFrameDeallocator(AVFrame* frame);
void AVPacketDeallocator(AVPacket* packet);
}  // namespace deallocator

namespace details {
#define OWNED_FFMPEG_RESOURCE(RESOURCE)                                 \
  class Owned##RESOURCE                                                 \
      : public std::unique_ptr<                                         \
            RESOURCE, decltype(&deallocator::RESOURCE##Deallocator)> {  \
   private:                                                             \
    using BaseType =                                                    \
        std::unique_ptr<RESOURCE,                                       \
                        decltype(&deallocator::RESOURCE##Deallocator)>; \
                                                                        \
   public:                                                              \
    using BaseType::unique_ptr;                                         \
    [[nodiscard]] Owned##RESOURCE(RESOURCE* r)                          \
        : BaseType{r, deallocator::RESOURCE##Deallocator} {}            \
  };

#define OWNED_SDL_RESOURCE(RESOURCE)                                       \
  class OwnedSdl##RESOURCE                                                 \
      : public std::unique_ptr<SDL_##RESOURCE,                             \
                               decltype(&SDL_Destroy##RESOURCE)> {         \
   private:                                                                \
    using BaseType =                                                       \
        std::unique_ptr<SDL_##RESOURCE, decltype(&SDL_Destroy##RESOURCE)>; \
                                                                           \
   public:                                                                 \
    using BaseType::unique_ptr;                                            \
    [[nodiscard]] OwnedSdl##RESOURCE(SDL_##RESOURCE* r)                    \
        : BaseType{r, SDL_Destroy##RESOURCE} {}                            \
  };

OWNED_FFMPEG_RESOURCE(AVFrame)
OWNED_FFMPEG_RESOURCE(AVPacket)

OWNED_SDL_RESOURCE(Window)
OWNED_SDL_RESOURCE(Renderer)
OWNED_SDL_RESOURCE(Texture)
}  // namespace details

using sdl_window = details::OwnedSdlWindow;
using sdl_renderer = details::OwnedSdlRenderer;
using sdl_texture = details::OwnedSdlTexture;

using av_frame = details::OwnedAVFrame;
using av_packet = details::OwnedAVPacket;

#undef OWNED_FFMPEG_RESOURCE
#undef OWNED_SDL_RESOURCE

}  // namespace mvplayer
