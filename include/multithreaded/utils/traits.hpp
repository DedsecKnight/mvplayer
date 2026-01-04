#pragma once

#include "events/handler.hpp"

namespace multithreaded::traits {
template <typename handler_t>
constexpr bool is_event_handler =
    std::is_base_of_v<events::any_handler, handler_t>;
};  // namespace multithreaded::traits
