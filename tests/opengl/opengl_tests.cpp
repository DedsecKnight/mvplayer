#include <gtest/gtest.h>

#include <SDL3/SDL.h>
#include <glad/gl.h>

#include <array>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "index_buffer.hpp"
#include "pixel_buffer.hpp"
#include "shader.hpp"
#include "texture.hpp"
#include "vertex_array.hpp"
#include "vertex_buffer.hpp"

class OpenGLTest : public ::testing::Test {
 protected:
  void SetUp() override {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
      GTEST_SKIP() << "Cannot init SDL: " << SDL_GetError();
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    window_ = SDL_CreateWindow("test", 1, 1,
                               SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (!window_) {
      GTEST_SKIP() << "Cannot create SDL window: " << SDL_GetError();
    }
    context_ = SDL_GL_CreateContext(window_);
    if (!context_) {
      GTEST_SKIP() << "Cannot create GL context: " << SDL_GetError();
    }
    gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress);
  }

  void TearDown() override {
    if (context_) {
      SDL_GL_DestroyContext(context_);
    }
    if (window_) {
      SDL_DestroyWindow(window_);
    }
    SDL_Quit();
  }

  SDL_Window* window_{nullptr};
  SDL_GLContext context_{nullptr};
};

// --- Texture tests ---

TEST_F(OpenGLTest, TextureCreation) {
  opengl::texture tex;
  EXPECT_NE(tex.id(), 0u);
}

TEST_F(OpenGLTest, TextureWithData) {
  std::array<uint8_t, 12> pixels{};
  opengl::texture_spec spec{.width = 2,
                             .height = 2,
                             .internal_format = GL_RGB,
                             .format = GL_RGB,
                             .data_type = GL_UNSIGNED_BYTE};
  opengl::texture tex{std::span{pixels}, spec};
  EXPECT_NE(tex.id(), 0u);
}

TEST_F(OpenGLTest, TextureMoveConstruction) {
  opengl::texture tex;
  auto original_id = tex.id();
  opengl::texture moved{std::move(tex)};
  EXPECT_EQ(tex.id(), 0u);
  EXPECT_EQ(moved.id(), original_id);
}

TEST_F(OpenGLTest, TextureMoveAssignment) {
  opengl::texture tex;
  auto original_id = tex.id();
  opengl::texture target;
  target = std::move(tex);
  EXPECT_EQ(tex.id(), 0u);
  EXPECT_EQ(target.id(), original_id);
}

// --- VertexBuffer tests ---

TEST_F(OpenGLTest, VertexBufferCreation) {
  opengl::vertex_buffer vbo;
  EXPECT_NE(vbo.id(), 0u);
}

TEST_F(OpenGLTest, VertexBufferWithData) {
  std::array<float, 6> data{0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 1.0f};
  opengl::vertex_buffer vbo{std::span{data}};
  EXPECT_NE(vbo.id(), 0u);
}

TEST_F(OpenGLTest, VertexBufferMoveConstruction) {
  opengl::vertex_buffer vbo;
  auto original_id = vbo.id();
  opengl::vertex_buffer moved{std::move(vbo)};
  EXPECT_EQ(vbo.id(), 0u);
  EXPECT_EQ(moved.id(), original_id);
}

// --- IndexBuffer tests ---

TEST_F(OpenGLTest, IndexBufferCreation) {
  opengl::index_buffer ibo;
  EXPECT_NE(ibo.id(), 0u);
}

TEST_F(OpenGLTest, IndexBufferWithData) {
  std::array<uint32_t, 3> indices{0, 1, 2};
  opengl::index_buffer ibo{std::span{indices}};
  EXPECT_NE(ibo.id(), 0u);
}

TEST_F(OpenGLTest, IndexBufferMoveConstruction) {
  opengl::index_buffer ibo;
  auto original_id = ibo.id();
  opengl::index_buffer moved{std::move(ibo)};
  EXPECT_EQ(ibo.id(), 0u);
  EXPECT_EQ(moved.id(), original_id);
}

// --- VertexArray tests ---

TEST_F(OpenGLTest, VertexArrayCreation) {
  opengl::vertex_array vao;
  EXPECT_NE(vao.id(), 0u);
}

TEST_F(OpenGLTest, VertexArrayWithAttributes) {
  std::array<float, 6> data{0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 1.0f};
  opengl::vertex_buffer vbo{std::span{data}};

  std::array<opengl::vertex_attribute_spec, 1> attrs{
      {{.component_size = 2,
        .data_type = GL_FLOAT,
        .normalized = GL_FALSE,
        .stride = static_cast<GLsizei>(2 * sizeof(float)),
        .offset = nullptr}}};
  opengl::vertex_array vao{std::span{attrs}};
  EXPECT_NE(vao.id(), 0u);
}

TEST_F(OpenGLTest, VertexArrayMoveConstruction) {
  opengl::vertex_array vao;
  auto original_id = vao.id();
  opengl::vertex_array moved{std::move(vao)};
  EXPECT_EQ(vao.id(), 0u);
  EXPECT_EQ(moved.id(), original_id);
}

// --- PixelBuffer tests ---

TEST_F(OpenGLTest, PixelBufferCreation) {
  opengl::pixel_buffer pbo;
  EXPECT_NE(pbo.id(), 0u);
}

TEST_F(OpenGLTest, PixelBufferMoveConstruction) {
  opengl::pixel_buffer pbo;
  auto original_id = pbo.id();
  opengl::pixel_buffer moved{std::move(pbo)};
  EXPECT_EQ(pbo.id(), 0u);
  EXPECT_EQ(moved.id(), original_id);
}

// --- Shader tests ---

namespace {
struct valid_vertex_shader {
  static constexpr GLenum type = GL_VERTEX_SHADER;
  static constexpr const char* source =
      "#version 430 core\n"
      "layout (location = 0) in vec2 aPos;\n"
      "void main() { gl_Position = vec4(aPos, 0.0, 1.0); }\n";
};

struct valid_fragment_shader {
  static constexpr GLenum type = GL_FRAGMENT_SHADER;
  static constexpr const char* source =
      "#version 430 core\n"
      "out vec4 FragColor;\n"
      "void main() { FragColor = vec4(1.0); }\n";
};

struct invalid_fragment_shader {
  static constexpr GLenum type = GL_FRAGMENT_SHADER;
  static constexpr const char* source = "this is not valid glsl";
};

struct incomplete_shader_spec {
  static constexpr GLenum type = GL_VERTEX_SHADER;
};
}  // namespace

TEST_F(OpenGLTest, ShaderCreation) {
  opengl::shader s{valid_vertex_shader{}, valid_fragment_shader{}};
  EXPECT_NE(s.id(), 0u);
}

TEST_F(OpenGLTest, ShaderInvalidSource) {
  EXPECT_THROW(
      opengl::shader(valid_vertex_shader{}, invalid_fragment_shader{}),
      std::runtime_error);
}

TEST_F(OpenGLTest, ShaderMoveConstruction) {
  opengl::shader s{valid_vertex_shader{}, valid_fragment_shader{}};
  auto original_id = s.id();
  opengl::shader moved{std::move(s)};
  EXPECT_EQ(s.id(), 0u);
  EXPECT_EQ(moved.id(), original_id);
}

TEST_F(OpenGLTest, ShaderSpecConcept) {
  static_assert(opengl::shader_spec_concept<valid_vertex_shader>);
  static_assert(opengl::shader_spec_concept<valid_fragment_shader>);
  static_assert(!opengl::shader_spec_concept<incomplete_shader_spec>);
}
