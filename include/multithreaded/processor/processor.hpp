#pragma once

#include <atomic>
#include <concepts>
#include <span>
#include <string_view>

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
    void add_read_port(std::string_view sender_name,
                       read::port<event_ts...>&& read_port) {
      processor_.add_read_port(sender_name, std::move(read_port));
    }

    template <typename... event_ts>
    void add_write_port(std::string_view recipient_name,
                        write::port<event_ts...>&& write_port) {
      processor_.add_write_port(recipient_name, std::move(write_port));
    }

    void start_event_loop(std::span<char* const> args) noexcept override {
      processor_.on_startup(args);
      using envelope_generator_t =
          typename processor_t::enveloped_event_generator_t;
      using event_holder_t = typename processor_t::event_receiver_t;
      event_holder_t event_holder;

      while (!terminated_.load(std::memory_order_relaxed)) {
        auto& output_queues = processor_.output_queues().get();
        for (auto&& [sender_name, rp] : processor_.input_queues().get()) {
          write::any* sender_mailbox{nullptr};
          auto it = output_queues.find(sender_name);
          if (it != output_queues.end()) {
            sender_mailbox = &it->second;
          }
          envelope_generator_t envelope_generator{sender_name, sender_mailbox};
          if (rp.get_event(event_holder)) {
            std::visit(envelope_generator, event_holder);
            std::visit(processor_, envelope_generator.get_enveloped_event());
          }
        }
      }
    }

   private:
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
