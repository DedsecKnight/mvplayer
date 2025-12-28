#pragma once

#include <SDL3/SDL.h>

#include "utils/owned.hpp"

namespace mvplayer {
class SdlManager {
 public:
  [[nodiscard]] static SdlManager& GetInstance() {
    static SdlManager s;
    return s;
  }

  SdlManager(const SdlManager&) = delete;
  SdlManager& operator=(const SdlManager&) = delete;

  [[nodiscard]] static OwnedSdlWindow CreateWindow(std::string_view windowName,
                                                   int width,
                                                   int height) noexcept {
    return OwnedSdlWindow{
        SDL_CreateWindow(windowName.data(), width, height,
                         SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS)};
  }

  [[nodiscard]] static OwnedSdlRenderer CreateRenderer(
      SDL_Window* window) noexcept {
    return OwnedSdlRenderer{SDL_CreateRenderer(window, nullptr)};
  }

  [[nodiscard]] static OwnedSdlTexture CreateTexture(
      SDL_Renderer* renderer, SDL_PixelFormat pixelFormat,
      SDL_TextureAccess textureAccess, int32_t width, int32_t height) noexcept {
    return OwnedSdlTexture{
        SDL_CreateTexture(renderer, pixelFormat, textureAccess, width, height)};
  }

 private:
  SdlManager() { SDL_Init(SDL_INIT_VIDEO); }
  ~SdlManager() { SDL_Quit(); }
};
}  // namespace mvplayer
