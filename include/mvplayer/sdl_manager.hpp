#pragma once

#include <SDL3/SDL.h>

#include "utils/owned.hpp"

namespace mvplayer {
class sdl_manager {
 public:
  [[nodiscard]] static sdl_manager& get_instance() {
    static sdl_manager s;
    return s;
  }

  sdl_manager(const sdl_manager&) = delete;
  sdl_manager& operator=(const sdl_manager&) = delete;

  [[nodiscard]] static sdl_window create_window(std::string_view window_name,
                                                int width,
                                                int height) noexcept {
    return get_instance().new_window(window_name, width, height);
  }

  [[nodiscard]] static sdl_renderer create_renderer(
      SDL_Window* window) noexcept {
    return get_instance().new_renderer(window);
  }

  [[nodiscard]] static sdl_texture create_texture(
      SDL_Renderer* renderer, SDL_PixelFormat pixel_format,
      SDL_TextureAccess texture_access, int32_t width,
      int32_t height) noexcept {
    return get_instance().new_texture(renderer, pixel_format, texture_access,
                                      width, height);
  }

 private:
  [[nodiscard]] sdl_window new_window(std::string_view window_name, int width,
                                      int height) noexcept {
    return sdl_window{
        SDL_CreateWindow(window_name.data(), width, height,
                         SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS)};
  }

  [[nodiscard]] sdl_renderer new_renderer(SDL_Window* window) noexcept {
    return sdl_renderer{SDL_CreateRenderer(window, nullptr)};
  }

  [[nodiscard]] sdl_texture new_texture(SDL_Renderer* renderer,
                                        SDL_PixelFormat pixel_format,
                                        SDL_TextureAccess texture_access,
                                        int32_t width,
                                        int32_t height) noexcept {
    return sdl_texture{SDL_CreateTexture(renderer, pixel_format, texture_access,
                                         width, height)};
  }

 private:
  sdl_manager() { SDL_Init(SDL_INIT_VIDEO); }
  ~sdl_manager() { SDL_Quit(); }
};
}  // namespace mvplayer
