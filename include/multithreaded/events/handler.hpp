#pragma once

#include <unordered_map>
#include <variant>

#include "connector/read/any.hpp"
#include "connector/write/any.hpp"
#include "events/envelope.hpp"
namespace multithreaded::events {

class any_handler {
 public:
  template <typename... event_ts>
  void add_read_port(std::string_view sender_name,
                     read::port<event_ts...>&& read_port) {
    input_queue_.emplace(sender_name, std::move(read_port));
  }

  template <typename... event_ts>
  void add_write_port(std::string_view recipient_name,
                      write::port<event_ts...>&& write_port) {
    output_queue_.emplace(recipient_name, std::move(write_port));
  }

  [[nodiscard]] auto input_queues() noexcept { return std::ref(input_queue_); }
  [[nodiscard]] auto output_queues() noexcept {
    return std::ref(output_queue_);
  }

  template <typename event_t>
  void broadcast(const event_t& e) noexcept {
    for (auto&& [recipient_name, wp] : output_queue_) {
      if (wp.send_event(e)) {
        spdlog::trace("Sucessfully sent event to {}", recipient_name);
      }
    }
  }

 private:
  std::unordered_map<std::string_view, read::any> input_queue_;
  std::unordered_map<std::string_view, write::any> output_queue_;
};

template <typename event_t>
class handler {
 public:
  handler() = default;
  virtual void operator()(const envelope<event_t>& val) = 0;
};

template <typename... event_ts>
class handlers : public any_handler, public handler<event_ts>... {
 public:
  using event_handler_t = handlers<event_ts...>;
  using event_receiver_t = std::variant<event_ts...>;
  using enveloped_event_generator_t = envelope_generator<event_ts...>;
  using handler<event_ts>::handler...;
  using handler<event_ts>::operator()...;
};
}  // namespace multithreaded::events
