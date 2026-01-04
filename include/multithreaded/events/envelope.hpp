#pragma once

#include <print>
#include <string_view>
#include <variant>

#include "connector/write/any.hpp"

namespace multithreaded::events {

template <typename payload_t>
class envelope {
 private:
  class sender_info {
   public:
    sender_info(write::any* sender_mailbox, std::string_view sender_name)
        : mailbox_{sender_mailbox}, name_{sender_name} {}

    [[nodiscard]] std::string_view name() const noexcept { return name_; }

    friend class envelope;

   private:
    write::any* mailbox_{nullptr};
    std::string_view name_;
  };

 public:
  envelope() : sender_{nullptr, ""}, payload_{} {}

  // TODO: get rid of payload copy here
  envelope(write::any* sender_mailbox, std::string_view sender_name,
           const payload_t& payload)
      : sender_{sender_mailbox, sender_name}, payload_{payload} {}

  [[nodiscard]] const payload_t& payload() const noexcept { return payload_; }
  [[nodiscard]] const sender_info& sender() const noexcept { return sender_; }

  template <typename event_t>
  void reply([[maybe_unused]] const event_t& e) const noexcept {
    if (sender_.mailbox_ == nullptr) {
      // TODO: figure out what to do here
      std::println("No mailbox found");
      return;
    }
    if (!sender_.mailbox_->send_event(e)) {
      std::println("Failed to send event {}", typeid(payload_t).name());
      // TODO: figure out what to do here. retries maybe?
    }
  }

 private:
  sender_info sender_;
  payload_t payload_;
};

template <typename... event_ts>
class envelope_generator {
 public:
  [[nodiscard]] envelope_generator(std::string_view sender_name,
                                   write::any* return_mailbox)
      : sender_name_{sender_name},
        return_mailbox_{return_mailbox},
        enveloped_event_{} {}

  template <typename payload_t>
  void operator()(const payload_t& payload) {
    if constexpr ((std::is_same_v<payload_t, event_ts> || ...)) {
      enveloped_event_ =
          envelope<payload_t>{return_mailbox_, sender_name_, payload};
    }
  }

  [[nodiscard]] const std::variant<envelope<event_ts>...>& get_enveloped_event()
      const noexcept {
    return enveloped_event_;
  }

 private:
  std::string_view sender_name_;
  write::any* return_mailbox_;
  std::variant<envelope<event_ts>...> enveloped_event_;
};

};  // namespace multithreaded::events
