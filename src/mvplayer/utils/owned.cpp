#include "utils/owned.hpp"

#include <SDL3/SDL_video.h>
#include <spdlog/spdlog.h>

namespace mvplayer::deallocator {
void AVFrameDeallocator(AVFrame* frame) { av_frame_free(&frame); }
void AVPacketDeallocator(AVPacket* packet) { av_packet_free(&packet); }
void SwrContextDeallocator(SwrContext* ctx) {
  if (ctx != nullptr) {
    swr_free(&ctx);
  }
}
void SDLGLContextDeallocator(SDL_GLContextState* ctx) {
  if (!SDL_GL_DestroyContext(ctx)) {
    spdlog::error("Failed to destroy OpenGL context: {}", SDL_GetError());
  }
}
}  // namespace mvplayer::deallocator
