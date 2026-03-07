#include "shader.hpp"

#include <glad/gl.h>

#include <utility>

namespace opengl {
shader::shader() : id_{glCreateProgram()} {}

void shader::use() const noexcept { glUseProgram(id_); }

shader::shader(shader&& other_shader) noexcept
    : id_{std::exchange(other_shader.id_, 0)} {}

shader& shader::operator=(shader&& other_shader) noexcept {
  id_ = std::exchange(other_shader.id_, 0);
  return *this;
}

shader::~shader() { glDeleteProgram(id_); }

[[nodiscard]] shader::shader_creation_result shader::attach_shader_component(
    GLuint shader_component_id) const noexcept {
  glAttachShader(id_, shader_component_id);
  return shader_component_id;
}

shader::shader_creation_result shader::shader_component_compilation_status(
    GLuint component_id) noexcept {
  int32_t success{};
  const size_t max_log_size{512};
  std::vector<char> info_log(max_log_size);

  glGetShaderiv(component_id, GL_COMPILE_STATUS, &success);
  if (success == 0) {
    glGetShaderInfoLog(component_id, static_cast<int32_t>(max_log_size),
                       nullptr, info_log.data());
    return std::unexpected(std::string{info_log.data()});
  }

  return component_id;
}
}  // namespace opengl
