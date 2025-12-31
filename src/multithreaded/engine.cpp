#include "engine.hpp"

namespace multithreaded {

engine::~engine() noexcept {
  for (auto& pt : processor_threads_) {
    if (pt.joinable()) {
      pt.join();
    }
  }
}

void engine::start(std::span<char* const>) noexcept {
  // for (const auto& [_, processor] : processor_registry_) {
  //   processor_threads_.emplace_back(&processor::start, &processor);
  // }
}
}  // namespace multithreaded
