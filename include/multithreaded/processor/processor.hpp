#pragma once

#include <concepts>
#include <span>
#include <string_view>
#include <vector>

#include "connector/read/any.hpp"
#include "connector/read/port.hpp"
#include "connector/write/any.hpp"
#include "connector/write/port.hpp"

namespace multithreaded {

// template <typename processor_t>
// class event_handlers {
//  public:
//   template <typename T>
//   void operator()(T&& val) {
//     static_cast<processor_t*>(this)->handle_event(std::forward<T>(val));
//   }
// };

class any_processor {
 private:
  using name_t = std::string_view;

  struct processor_concept {
    virtual ~processor_concept() = default;
    virtual void start_event_loop(
        std::span<char* const> args) const noexcept = 0;
  };

  template <typename processor_t>
  class processor_model : public processor_concept {
   private:
    using event_handler_t = typename processor_t::event_handler_t;

   public:
    template <typename... arg_ts>
      requires std::constructible_from<processor_t, arg_ts...>
    processor_model(arg_ts&&... args)
        : processor_(std::forward<arg_ts>(args)...) {}

    template <typename... event_ts>
    void add_read_port(read::port<event_ts...>&& read_port) {
      input_queue_.emplace_back(std::move(read_port));
    }

    template <typename... event_ts>
    void add_write_port(write::port<event_ts...>&& write_port) {
      output_queue_.emplace_back(std::move(write_port));
    }

    template <typename event_t>
    void handle_event(event_t&& e) {
      processor_.handle_event(std::forward<event_t>(e));
    }

    void start_event_loop(std::span<char* const> args) const noexcept override {
      processor_.on_startup(args);
      while (true) {
        for ([[maybe_unused]] const auto& rp : input_queue_) {
          // processor_.process_incoming_event(rp);
          // TODO: figure out what to do here to process input from queue
        }
      }
    }

   private:
    std::vector<read::any> input_queue_;
    std::vector<write::any> output_queue_;
    processor_t processor_;
  };

 public:
  template <typename processor_t>
  any_processor(processor_t&& processor)
      : pimpl_{std::make_unique<processor_model<processor_t>>(
            std::move(processor))} {}

  template <typename processor_t>
  processor_model<processor_t>& as() {
    auto processor_ptr =
        dynamic_cast<processor_model<processor_t>*>(pimpl_.get());
    if (!processor_ptr) {
      // TODO: Consider a better way to handle error instead of throwing
      throw std::bad_cast{};
    }
    return *processor_ptr;
  }

  void start(std::span<char* const> args) const noexcept {
    pimpl_->start_event_loop(args);
  }

 private:
  std::unique_ptr<processor_concept> pimpl_;
};
}  // namespace multithreaded
