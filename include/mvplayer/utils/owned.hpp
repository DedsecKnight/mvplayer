#pragma once

#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
}

#include <SDL3/SDL.h>

namespace mvplayer {

namespace deallocator {
void AVFrameDeallocator(AVFrame* frame);
void AVPacketDeallocator(AVPacket* packet);
void SwrContextDeallocator(SwrContext* ctx);
void SDLGLContextDeallocator(SDL_GLContextState* ctx);
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
    [[nodiscard]] explicit Owned##RESOURCE(RESOURCE* raw_resource)      \
        : BaseType{raw_resource, deallocator::RESOURCE##Deallocator} {} \
  };

#define OWNED_SDL_RESOURCE(RESOURCE)                                        \
  class OwnedSdl##RESOURCE                                                  \
      : public std::unique_ptr<SDL_##RESOURCE,                              \
                               decltype(&SDL_Destroy##RESOURCE)> {          \
   private:                                                                 \
    using BaseType =                                                        \
        std::unique_ptr<SDL_##RESOURCE, decltype(&SDL_Destroy##RESOURCE)>;  \
                                                                            \
   public:                                                                  \
    using BaseType::unique_ptr;                                             \
    [[nodiscard]] explicit OwnedSdl##RESOURCE(SDL_##RESOURCE* raw_resource) \
        : BaseType{raw_resource, SDL_Destroy##RESOURCE} {}                  \
  };

OWNED_FFMPEG_RESOURCE(AVFrame)
OWNED_FFMPEG_RESOURCE(AVPacket)
OWNED_FFMPEG_RESOURCE(SwrContext)

OWNED_SDL_RESOURCE(Window)
OWNED_SDL_RESOURCE(Renderer)
OWNED_SDL_RESOURCE(Texture)
OWNED_SDL_RESOURCE(AudioStream)
}  // namespace details

class sdl_gl_context
    : public std::unique_ptr<SDL_GLContextState,
                             decltype(&deallocator::SDLGLContextDeallocator)> {
 private:
  using base_type =
      std::unique_ptr<SDL_GLContextState,
                      decltype(&deallocator::SDLGLContextDeallocator)>;

 public:
  using base_type::unique_ptr;
  [[nodiscard]] explicit sdl_gl_context(SDL_GLContext raw_resource)
      : base_type{raw_resource, deallocator::SDLGLContextDeallocator} {}
};

using sdl_window = details::OwnedSdlWindow;
using sdl_renderer = details::OwnedSdlRenderer;
using sdl_texture = details::OwnedSdlTexture;
using sdl_audio_stream = details::OwnedSdlAudioStream;

using av_frame = details::OwnedAVFrame;
using av_packet = details::OwnedAVPacket;
using swr_context = details::OwnedSwrContext;

#undef OWNED_FFMPEG_RESOURCE
#undef OWNED_SDL_RESOURCE

}  // namespace mvplayer
