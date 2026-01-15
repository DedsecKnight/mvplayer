#pragma once

#include <atomic>
#include <concepts>
#include <span>
#include <string_view>

#include "connector/read/port.hpp"
#include "connector/write/any.hpp"
#include "connector/write/port.hpp"
#include "processor/termination_handler.hpp"

namespace multithreaded::processor {

class any {
 private:
  using name_t = std::string_view;

  struct container {  // NOLINT
    virtual ~container() = default;
    virtual void start_event_loop(std::span<char* const> args) noexcept = 0;
    [[nodiscard]] virtual bool terminated() const noexcept = 0;
    virtual void terminate() noexcept = 0;
  };

  template <typename processor_t>
  class model : public container {
   private:
    using event_handler_t = typename processor_t::event_handler_t;

   public:
    void terminate() noexcept override {
      auto* terminate_handler_ptr =
          dynamic_cast<processor::termination_handler*>(&processor_);
      if (terminate_handler_ptr != nullptr) {
        terminate_handler_ptr->handle_termination_signal();
      }
      terminated_.store(true, std::memory_order_release);
    }

    [[nodiscard]] bool terminated() const noexcept override {
      return terminated_.load(std::memory_order_acquire);
    }

    template <typename... arg_ts>
      requires std::constructible_from<processor_t, arg_ts...>
    explicit model(arg_ts&&... args)
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

      while (!terminated_.load(std::memory_order_acquire)) {
        auto& output_queues = processor_.output_queues().get();
        for (auto&& [sender_name, rp] : processor_.input_queues().get()) {
          write::any* sender_mailbox{nullptr};
          auto it = output_queues.find(sender_name);  // NOLINT
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
    requires(!std::is_same_v<any, std::decay_t<processor_t>>)
  explicit any(processor_t&& processor)
      : pimpl_{std::make_unique<model<processor_t>>(
            std::forward<processor_t>(processor))} {}

  template <typename processor_t>
  [[nodiscard]] model<processor_t>& as() {
    auto processor_ptr = dynamic_cast<model<processor_t>*>(pimpl_.get());
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
  [[nodiscard]] bool terminated() const noexcept {
    return pimpl_->terminated();
  }

 private:
  std::unique_ptr<container> pimpl_;
};
}  // namespace multithreaded::processor
