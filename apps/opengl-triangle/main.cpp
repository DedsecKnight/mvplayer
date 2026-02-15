// clang-format off
#include <glad/gl.h>
#include <SDL3/SDL.h>
// clang-format on

#include <expected>
#include <print>
#include <vector>

using namespace std::string_literals;

static constexpr int WIDTH = 800;
static constexpr int HEIGHT = 600;

static constexpr const char* VERTEX_SHADER = R"(
#version 330 core
layout (location = 0) in vec3 v_pos;

void main() {
  gl_Position = vec4(v_pos.x, v_pos.y, v_pos.z, 1.0);
}
)";

static constexpr const char* FRAGMENT_SHADER = R"(
#version 330 core
out vec4 pixel_color;

void main() {
  pixel_color = vec4(1.0f, 0.5f, 0.2f, 1.0f);
}
)";

namespace {
std::expected<int, std::string> get_shader_compile_status(uint32_t shader) {
  int success{};
  const size_t max_log_size = 512;
  std::vector<char> info_log(max_log_size);
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (success == 0) {
    glGetShaderInfoLog(shader, static_cast<int>(info_log.size()), nullptr,
                       info_log.data());
    return std::unexpected(std::string{info_log.data()});
  }
  return success;
}

uint32_t create_shader(int shader_type, const char* shader_source) {
  auto shader = glCreateShader(shader_type);
  glShaderSource(shader, 1, &shader_source, nullptr);
  glCompileShader(shader);

  if (auto compile_status = get_shader_compile_status(shader);
      !compile_status.has_value()) {
    std::println("Error compiling shader: {}", compile_status.error());
    return -1;
  }

  return shader;
}
}  // namespace

int main() {
  SDL_Init(SDL_INIT_VIDEO);

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  SDL_Window* window{nullptr};
  SDL_Renderer* renderer{nullptr};

  SDL_CreateWindowAndRenderer(
      "GL with SDL3", WIDTH, HEIGHT,
      SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS, &window,
      &renderer);

  SDL_GLContext context = SDL_GL_CreateContext(window);

  int version = gladLoadGL(static_cast<GLADloadfunc>(SDL_GL_GetProcAddress));
  std::println("GL {}.{}", GLAD_VERSION_MAJOR(version),
               GLAD_VERSION_MINOR(version));

  const std::vector<float> vertices{-0.5F, -0.5F, 0.0F, 0.5F, -0.5F,
                                    0.0F,  0.0F,  0.5F, 0.0F};

  uint32_t vertex_array_object{};
  uint32_t vertex_buffer_object{};
  glGenVertexArrays(1, &vertex_array_object);
  glGenBuffers(1, &vertex_buffer_object);

  glBindVertexArray(vertex_array_object);

  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_object);
  glBufferData(GL_ARRAY_BUFFER,
               static_cast<int64_t>(vertices.size() * sizeof(float)),
               vertices.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float),
                        static_cast<void*>(0));
  glEnableVertexAttribArray(0);

  auto vertex_shader = ::create_shader(GL_VERTEX_SHADER, VERTEX_SHADER);
  auto fragment_shader = ::create_shader(GL_FRAGMENT_SHADER, FRAGMENT_SHADER);

  auto shader_program = glCreateProgram();
  glAttachShader(shader_program, vertex_shader);
  glAttachShader(shader_program, fragment_shader);
  glLinkProgram(shader_program);
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

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

    glClearColor(0.2F, 0.3F, 0.3F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shader_program);
    glBindVertexArray(vertex_array_object);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    SDL_GL_SwapWindow(window);
    SDL_Delay(1);
  }

  glDeleteVertexArrays(1, &vertex_array_object);
  glDeleteBuffers(1, &vertex_buffer_object);
  glDeleteProgram(shader_program);

  SDL_GL_DestroyContext(context);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  return 0;
}
