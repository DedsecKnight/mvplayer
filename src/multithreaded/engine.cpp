#include "engine.hpp"

#include "processor/processor.hpp"

namespace multithreaded {

engine::~engine() noexcept {
  for (const auto& [_, processor] : processor_registry_) {
    processor.terminate();
  }
  for (auto& pt : processor_threads_) {
    if (pt.joinable()) {
      pt.join();
    }
  }
}

void engine::start(std::span<char* const> args) noexcept {
  for (const auto& [_, processor] : processor_registry_) {
    processor_threads_.emplace_back(&any_processor::start, &processor, args);
  }
}
}  // namespace multithreaded
