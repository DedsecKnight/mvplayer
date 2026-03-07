// clang-format off
#include <glad/gl.h>
#include <SDL3/SDL.h>
#include "texture.hpp"
// clang-format on
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <print>
#include <vector>

#include "shader.hpp"
#include "vertex_array.hpp"
#include "vertex_buffer.hpp"

static constexpr int WIDTH = 800;
static constexpr int HEIGHT = 600;

struct vertex_shader_spec {
  static constexpr const int32_t type = GL_VERTEX_SHADER;
  static constexpr const char* source = R"(
    #version 430 core
    layout (location = 0) in vec3 v_pos;
    layout (location = 1) in vec2 tex_pos;

    out vec2 tex_coord;

    void main() {
      gl_Position = vec4(v_pos.x, v_pos.y, v_pos.z, 1.0);
      tex_coord = tex_pos;
    }
  )";
};

struct fragment_shader_spec {
  static constexpr int32_t type = GL_FRAGMENT_SHADER;
  static constexpr const char* source = R"(
    #version 430 core
    in vec2 tex_coord;
    out vec4 pixel_color;

    uniform sampler2D image_texture;

    void main() {
      pixel_color = texture(image_texture, tex_coord);
    }
  )";
};

int main() {
  SDL_Init(SDL_INIT_VIDEO);

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  auto* window = SDL_CreateWindow(
      "GL with SDL3", WIDTH, HEIGHT,
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS);

  SDL_GLContext context = SDL_GL_CreateContext(window);

  int version = gladLoadGL(static_cast<GLADloadfunc>(SDL_GL_GetProcAddress));
  std::println("GL {}.{}", GLAD_VERSION_MAJOR(version),
               GLAD_VERSION_MINOR(version));

  const std::vector<float> vertices{-0.5F, -0.5F, 0.0F, 0.0F, 0.0F,
                                    0.5F,  -0.5F, 0.0F, 1.0F, 0.0F,
                                    0.0F,  0.5F,  0.0F, 0.5F, 1.0F};
  const std::vector<opengl::vertex_attribute_spec> attributes{
      {.component_size = 3,
       .data_type = GL_FLOAT,
       .normalized = GL_FALSE,
       .stride = 5 * sizeof(float),
       .offset = nullptr},
      {.component_size = 2,
       .data_type = GL_FLOAT,
       .normalized = GL_FALSE,
       .stride = 5 * sizeof(float),
       .offset = reinterpret_cast<void*>(3 * sizeof(float))}};  // NOLINT

  int32_t width{};
  int32_t height{};
  int32_t num_channels{};
  uint8_t* pixel_data = stbi_load("apps/opengl-triangle/container.jpg", &width,
                                  &height, &num_channels, 0);

  opengl::vertex_buffer vbo{vertices};
  opengl::vertex_array vao{attributes};
  std::span<uint8_t> pixel_data_view{
      pixel_data,
      pixel_data + (sizeof(uint8_t) * width * height * num_channels)};

  opengl::texture texture{pixel_data_view,
                          GL_RGB,
                          {.width = width, .height = height, .padding = 0}};
  stbi_image_free(pixel_data);

  opengl::shader shader_program{vertex_shader_spec{}, fragment_shader_spec{}};
  shader_program.use();

  bool exit = false;
  while (!exit) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        case SDL_EVENT_QUIT: {
          exit = true;
          break;
        }
        case SDL_EVENT_KEY_DOWN: {
          if (event.key.key == SDLK_ESCAPE) {
            exit = true;
          }
          break;
        }
        default:
          break;
      }
    }

    glClearColor(0.2F, 0.3F, 0.3F, 1.0F);  // NOLINT
    glClear(GL_COLOR_BUFFER_BIT);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    SDL_GL_SwapWindow(window);
    SDL_Delay(1);
  }

  SDL_GL_DestroyContext(context);
  SDL_DestroyWindow(window);

  return 0;
}
