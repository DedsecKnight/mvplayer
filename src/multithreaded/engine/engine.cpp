#include "engine/engine.hpp"

#include <atomic>
#include <variant>

#include "processor/any.hpp"
#include "utils/constants.hpp"

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
    if (!main_processor_.has_value() ||
        processor_name != main_processor_.value()) {
      spdlog::info("Starting {}", processor_name);
      processor_threads_.emplace_back(&processor::any::start, &processor, args);
    }
  }
  if (!main_processor_.has_value()) {
    start_event_loop();
  } else {
    processor_threads_.emplace_back(&engine::start_event_loop, this);
    spdlog::info("Starting {} as main processor", main_processor_.value());
    processor_registry_.at(main_processor_.value()).start(args);
  }
}

void engine::start_event_loop() noexcept {
  while (!is_terminated_.load(std::memory_order_acquire)) {
    for (auto& system_event_port : system_event_read_ports_) {
      std::variant<system_events::system_terminate_request_event>
          system_event_holder;
      if (system_event_port.pop(system_event_holder) &&
          std::holds_alternative<system_events::system_terminate_request_event>(
              system_event_holder)) {
        spdlog::info(
            "Received terminate request. Sending terminate signal to all "
            "processors.");
        for (auto&& [processor_name, processor] : processor_registry_) {
          spdlog::info("Sending termination signal to {}", processor_name);
          processor.terminate();
        }
        is_terminated_.store(true, std::memory_order_release);
        break;
      }
      std::this_thread::sleep_for(constants::SYSTEM_SIGNAL_POLL_INTERVAL);
    }
  }
}
}  // namespace multithreaded
