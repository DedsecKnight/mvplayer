#pragma once

#include <concepts>
#include <string_view>
#include <vector>

#include "connector.hpp"

namespace multithreaded {

template <typename processor_t>
class event_handlers {
 public:
  template <typename T>
  void operator()(T&& val) {
    static_cast<processor_t*>(this)->handle_event(std::forward<T>(val));
  }
};

class any_processor {
 private:
  using name_t = std::string_view;

  struct processor_concept {
    virtual ~processor_concept() = default;
  };

  template <typename processor_t>
  class processor_model : public processor_concept,
                          public event_handlers<processor_t> {
   public:
    template <typename... arg_ts>
      requires std::constructible_from<processor_t, arg_ts...>
    processor_model(arg_ts&&... args)
        : processor_(std::forward<arg_ts>(args)...) {}

    template <typename... event_ts>
    void add_read_port(read_only_port<event_ts...>&& read_port) {
      input_queue_.emplace_back(std::move(read_port));
    }

    template <typename... event_ts>
    void add_write_port(write_only_port<event_ts...>&& write_port) {
      output_queue_.emplace_back(std::move(write_port));
    }

    template <typename event_t>
    void handle_event(event_t&& e) {
      processor_.handle_event(std::forward<event_t>(e));
    }

    void start() noexcept {}

   private:
    std::vector<any_read_port> input_queue_;
    std::vector<any_write_port> output_queue_;
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

 private:
  std::unique_ptr<processor_concept> pimpl_;
};
}  // namespace multithreaded
