#pragma once

#include <atomic>
#include <concepts>
#include <ranges>
#include <span>
#include <string_view>

#include "connector/read/port.hpp"
#include "connector/write/any.hpp"
#include "connector/write/port.hpp"
#include "processor/interruptible.hpp"
#include "processor/pre_event_handler.hpp"
#include "processor/termination_handler.hpp"
#include "utils/constants.hpp"

namespace multithreaded::processor {

class any {
 private:
  using name_t = std::string_view;

  enum class termination_state : uint8_t {
    not_terminated,
    terminating,
    terminated
  };

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
      if (terminated_.load(std::memory_order_acquire) ==
          termination_state::terminating) {
        return;
      }
      terminated_.store(termination_state::terminating,
                        std::memory_order_release);
      if constexpr (std::is_base_of_v<processor::termination_handler,
                                      processor_t>) {
        processor_.handle_termination_signal();
      }
      terminated_.store(termination_state::terminated,
                        std::memory_order_release);
    }

    [[nodiscard]] bool terminated() const noexcept override {
      return terminated_.load(std::memory_order_acquire) ==
             termination_state::terminated;
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
      using envelope_generator_t =
          typename processor_t::enveloped_event_generator_t;
      using event_holder_t = typename processor_t::event_receiver_t;
      namespace ranges = std::ranges;

      processor_.on_startup(args);
      event_holder_t event_holder;

      std::vector<envelope_generator_t> sender_mailboxes;

      auto& output_queues = processor_.output_queues().get();
      auto& input_queues = processor_.input_queues().get();

      for (auto&& [sender_name, rp] : input_queues) {
        write::any* mailbox{nullptr};
        auto queue_it =
            ranges::find_if(output_queues, [&sender_name](const auto& port) {
              return port.first == sender_name;
            });
        if (queue_it != output_queues.end()) {
          mailbox = &queue_it->second;
        }
        sender_mailboxes.emplace_back(sender_name, mailbox);
      }

      while (terminated_.load(std::memory_order_acquire) ==
             termination_state::not_terminated) {
        if constexpr (std::is_base_of_v<processor::pre_event_handler,
                                        processor_t>) {
          processor_.pre_event();
        }
        if constexpr (std::is_base_of_v<processor::interruptible_processor,
                                        processor_t>) {
          if (terminated_.load(std::memory_order_acquire) !=
              termination_state::not_terminated) {
            return;
          }
          if (processor_.halt_requested()) {
            continue;
          }
        }
        for (auto&& [input_queue, sender_mailbox] :
             std::views::zip(input_queues, sender_mailboxes)) {
          if (input_queue.second.get_event(event_holder)) {
            std::visit(sender_mailbox, event_holder);
            std::visit(processor_, sender_mailbox.get_enveloped_event());
          }
        }
        std::this_thread::sleep_for(constants::EVENT_POLL_INTERVAL);
      }
    }

   private:
    std::atomic<termination_state> terminated_{
        termination_state::not_terminated};
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
