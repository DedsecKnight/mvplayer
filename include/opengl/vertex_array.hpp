#pragma once

#include <glad/gl.h>

#include <functional>
#include <span>
namespace opengl {

struct vertex_attribute_spec {
  GLint component_size;
  GLenum data_type;
  GLboolean normalized;
  GLsizei stride;
  void* offset;
};

class vertex_array {
 public:
  vertex_array();
  explicit vertex_array(std::span<const vertex_attribute_spec> attributes);

  void bind() const noexcept;
  void unbind() const noexcept;
  [[nodiscard]] GLuint id() const noexcept;
  void apply_attribute(const vertex_attribute_spec& attribute);

  template <typename... arg_ts>
    requires std::invocable<decltype(glVertexAttribPointer), GLuint, arg_ts...>
  void add_attribute(arg_ts&&... args) {
    bind();
    std::invoke(glVertexAttribPointer, num_attributes_,
                std::forward<arg_ts>(args)...);
    glEnableVertexAttribArray(num_attributes_++);
  }

  vertex_array(const vertex_array&) = delete;
  vertex_array& operator=(const vertex_array&) = delete;

  vertex_array(vertex_array&&) noexcept;
  vertex_array& operator=(vertex_array&&) noexcept;

  ~vertex_array();

 private:
  GLuint id_{0}, num_attributes_{0};
  mutable bool binded_{false};
};
}  // namespace opengl
