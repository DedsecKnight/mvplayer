#include "engine/engine.hpp"

#include <atomic>
#include <variant>

#include "processor/any.hpp"

namespace multithreaded {

engine::~engine() noexcept {
  for (auto&& [name, processor] : processor_registry_) {
    if (!processor.terminated()) {
      processor.terminate();
    }
  }
  for (auto& processor_thread : processor_threads_) {
    if (processor_thread.joinable()) {
      processor_thread.join();
    }
  }
}

void engine::start(std::span<char* const> args) noexcept {
  for (const auto& [processor_name, processor] : processor_registry_) {
    spdlog::info("Starting {}", processor_name);
    processor_threads_.emplace_back(&processor::any::start, &processor, args);
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
        for (auto&& [processor_name, processor] : processor_registry_) {
          spdlog::info("Sending termination signal to {}", processor_name);
          processor.terminate();
        }
        is_terminated_.store(true, std::memory_order_relaxed);
        break;
      }
    }
  }
}
}  // namespace multithreaded
