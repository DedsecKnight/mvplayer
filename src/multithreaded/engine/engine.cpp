#include "engine/engine.hpp"

#include <atomic>
#include <csignal>
#include <variant>

#include "processor/processor.hpp"

namespace multithreaded {

engine::~engine() noexcept {
  for (auto&& [name, processor] : processor_registry_) {
    if (!processor.terminated()) {
      processor.terminate();
    }
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
  while (!is_terminated_.load(std::memory_order_relaxed)) {
    for (auto& system_event_port : system_event_read_ports_) {
      std::variant<system_events::system_terminate_request_event>
          system_event_holder;
      if (!system_event_port.pop(system_event_holder)) {
        continue;
      }
      if (std::holds_alternative<system_events::system_terminate_request_event>(
              system_event_holder)) {
        spdlog::info(
            "Received terminate request. Sending terminate signal to all "
            "processors.");
        for (auto&& [_, processor] : processor_registry_) {
          processor.terminate();
        }
        is_terminated_.store(true, std::memory_order_relaxed);
        break;
      }
    }
  }
}
}  // namespace multithreaded
