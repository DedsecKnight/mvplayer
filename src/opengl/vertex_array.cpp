#include "vertex_array.hpp"

#include <glad/gl.h>

#include <algorithm>
#include <functional>
#include <utility>

namespace opengl {
vertex_array::vertex_array() { glGenVertexArrays(1, &id_); }

vertex_array::vertex_array(std::span<const vertex_attribute_spec> attributes)
    : vertex_array() {
  bind();
  namespace ranges = std::ranges;
  ranges::for_each(attributes,
                   std::bind_front(&vertex_array::apply_attribute, this));
}

vertex_array::vertex_array(vertex_array&& vao) noexcept
    : id_{std::exchange(vao.id_, 0)},
      binded_{std::exchange(vao.binded_, false)} {}

vertex_array& vertex_array::operator=(vertex_array&& vao) noexcept {
  id_ = std::exchange(vao.id_, 0);
  binded_ = std::exchange(vao.binded_, false);
  return *this;
}

void vertex_array::apply_attribute(const vertex_attribute_spec& attribute) {
  bind();
  glVertexAttribPointer(num_attributes_, attribute.component_size,
                        attribute.data_type, attribute.normalized,
                        attribute.stride, attribute.offset);
  glEnableVertexAttribArray(num_attributes_++);
}

vertex_array::~vertex_array() { glDeleteVertexArrays(1, &id_); }

void vertex_array::bind() const noexcept {
  if (!binded_) {
    glBindVertexArray(id_);
    binded_ = true;
  }
}

void vertex_array::unbind() const noexcept {
  if (binded_) {
    glBindVertexArray(0);
    binded_ = false;
  }
}
}  // namespace opengl
