#pragma once

// clang-format off
#include <glad/gl.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
// clang-format on
#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_video.h>
#include <spdlog/spdlog.h>

#include <string>

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
                                                int32_t width,
                                                int32_t height) noexcept {
    return get_instance().new_window(window_name, width, height);
  }

  [[nodiscard]] static sdl_gl_context create_gl_context(
      SDL_Window* window) noexcept {
    return get_instance().new_gl_context(window);
  }

  [[nodiscard]] static sdl_audio_stream create_audio_stream(
      SDL_AudioFormat audio_format, int32_t num_channels,
      int32_t frequency) noexcept {
    return get_instance().new_audio_stream(audio_format, num_channels,
                                           frequency);
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
  [[nodiscard]] sdl_window new_window(std::string_view window_name,
                                      int32_t width, int32_t height) {
    std::string safe_window_name{window_name};
    return sdl_window{SDL_CreateWindow(
        safe_window_name.c_str(), width, height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS)};
  }

  // NOLINTNEXTLINE
  [[nodiscard]] sdl_gl_context new_gl_context(SDL_Window* window) {
    return sdl_gl_context{SDL_GL_CreateContext(window)};
  }

  // NOLINTNEXTLINE
  [[nodiscard]] sdl_audio_stream new_audio_stream(SDL_AudioFormat audio_format,
                                                  int32_t num_channels,
                                                  int32_t frequency) {
    const SDL_AudioSpec audio_spec{
        .format = audio_format, .channels = num_channels, .freq = frequency};
    return sdl_audio_stream{SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &audio_spec, nullptr, nullptr)};
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

  sdl_manager() {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
  }
  ~sdl_manager() { SDL_Quit(); }
};
}  // namespace mvplayer
