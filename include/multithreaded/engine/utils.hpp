#pragma once

#include <string_view>
namespace multithreaded::system_events {
struct system_terminate_request_event {};
}  // namespace multithreaded::system_events

namespace multithreaded::constants {
static constexpr std::string_view ENGINE_IDENTIFIER = "main-engine";
}
