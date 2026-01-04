#pragma once

#include <atomic>
#include <concepts>
#include <flat_map>
#include <span>
#include <string_view>
#include <vector>

#include "connector/read/any.hpp"
#include "connector/read/port.hpp"
#include "connector/write/any.hpp"
#include "connector/write/port.hpp"

namespace multithreaded {

class any_processor {
 private:
  using name_t = std::string_view;

  struct processor_concept {
    virtual ~processor_concept() = default;
    virtual void start_event_loop(std::span<char* const> args) noexcept = 0;
    virtual void terminate() noexcept = 0;
  };

  template <typename processor_t>
  class processor_model : public processor_concept {
   private:
    using event_handler_t = typename processor_t::event_handler_t;

   public:
    void terminate() noexcept override {
      terminated_.store(true, std::memory_order_relaxed);
    }

    template <typename... arg_ts>
      requires std::constructible_from<processor_t, arg_ts...>
    processor_model(arg_ts&&... args)
        : processor_(std::forward<arg_ts>(args)...) {}

    template <typename... event_ts>
    void add_read_port(read::port<event_ts...>&& read_port) {
      input_queue_.emplace_back(std::move(read_port));
    }

    template <typename... event_ts>
    void add_write_port(std::string_view sender_name,
                        write::port<event_ts...>&& write_port) {
      output_queue_.emplace(sender_name, std::move(write_port));
    }

    void start_event_loop(std::span<char* const> args) noexcept override {
      processor_.on_startup(args);
      using event_holder_t = typename processor_t::event_receiver_t;
      event_holder_t event_holder;

      while (!terminated_.load(std::memory_order_relaxed)) {
        for (auto& rp : input_queue_) {
          if (rp.get_event(event_holder)) {
            std::visit(processor_, event_holder);
          }
        }
      }
    }

   private:
    std::vector<read::any> input_queue_;
    std::flat_map<std::string_view, write::any> output_queue_;
    std::atomic<bool> terminated_{false};
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

  void terminate() noexcept { pimpl_->terminate(); }

 private:
  std::unique_ptr<processor_concept> pimpl_;
};
}  // namespace multithreaded
