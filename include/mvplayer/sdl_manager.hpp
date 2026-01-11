#pragma once

#include <SDL3/SDL.h>

#include "utils/owned.hpp"

namespace mvplayer {
class sdl_manager {
 public:
  [[nodiscard]] static sdl_manager& get_instance() {
    static sdl_manager manager{};
    return manager;
  }

  sdl_manager(const sdl_manager&) = delete;
  sdl_manager& operator=(const sdl_manager&) = delete;

  sdl_manager(sdl_manager&&) = delete;
  sdl_manager& operator=(sdl_manager&&) = delete;

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
  // NOLINTNEXTLINE
  [[nodiscard]] sdl_window new_window(std::string_view window_name, int width,
                                      int height) {
    std::string safe_window_name{window_name};
    return sdl_window{
        SDL_CreateWindow(safe_window_name.c_str(), width, height,
                         SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS)};
  }

  // NOLINTNEXTLINE
  [[nodiscard]] sdl_renderer new_renderer(SDL_Window* window) noexcept {
    return sdl_renderer{SDL_CreateRenderer(window, nullptr)};
  }

  // NOLINTNEXTLINE
  [[nodiscard]] sdl_texture new_texture(SDL_Renderer* renderer,
                                        SDL_PixelFormat pixel_format,
                                        SDL_TextureAccess texture_access,
                                        int32_t width,
                                        int32_t height) noexcept {
    return sdl_texture{SDL_CreateTexture(renderer, pixel_format, texture_access,
                                         width, height)};
  }

  sdl_manager() { SDL_Init(SDL_INIT_VIDEO); }
  ~sdl_manager() { SDL_Quit(); }
};
}  // namespace mvplayer
