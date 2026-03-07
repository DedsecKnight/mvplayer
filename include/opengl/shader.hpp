#pragma once

#include <glad/gl.h>

#include <concepts>
#include <expected>
#include <format>
#include <functional>
#include <stdexcept>
#include <string>

namespace opengl {
template <typename T>
concept shader_spec_concept = requires(const T& shader) {
  { T::type } -> std::convertible_to<int>;
  { T::source } -> std::convertible_to<const char*>;
};

class shader {
 private:
  using shader_creation_result = std::expected<GLuint, std::string>;

 public:
  shader();

  template <shader_spec_concept vertex_shader_info_t,
            shader_spec_concept fragment_shader_info_t>
  shader([[maybe_unused]] const vertex_shader_info_t& vertex_shader_info,
         [[maybe_unused]] const fragment_shader_info_t& fragment_shader_info)
      : shader() {
    const auto attach_shader_fn =
        std::bind_front(&shader::attach_shader_component, this);

    const auto vertex_shader =
        create_shader_component<vertex_shader_info_t>().and_then(
            attach_shader_fn);
    if (!vertex_shader.has_value()) {
      throw std::runtime_error{std::format("error creating vertex shader: {}",
                                           vertex_shader.error())};
    }

    const auto fragment_shader =
        create_shader_component<fragment_shader_info_t>().and_then(
            attach_shader_fn);
    if (!fragment_shader.has_value()) {
      glDeleteShader(vertex_shader.value());
      throw std::runtime_error{std::format("error creating fragment shader: {}",
                                           fragment_shader.error())};
    }

    glLinkProgram(id_);
    glDeleteShader(fragment_shader.value());
    glDeleteShader(vertex_shader.value());
  }

  void use() const noexcept;

  shader(const shader&) = delete;
  shader& operator=(const shader&) = delete;

  shader(shader&&) noexcept;
  shader& operator=(shader&&) noexcept;

  ~shader();

 private:
  [[nodiscard]] shader_creation_result attach_shader_component(
      GLuint shader_component_id) const noexcept;

  static shader_creation_result shader_component_compilation_status(
      GLuint component_id) noexcept;

  template <shader_spec_concept shader_info_t>
  [[nodiscard]] shader_creation_result create_shader_component() {
    auto component_id = glCreateShader(shader_info_t::type);
    glShaderSource(component_id, 1, &shader_info_t::source, nullptr);
    glCompileShader(component_id);
    return shader_component_compilation_status(component_id);
  }

  GLuint id_;
};
}  // namespace opengl
