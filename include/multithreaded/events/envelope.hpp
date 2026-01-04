#pragma once

#include <string_view>
#include <variant>

namespace multithreaded::events {

template <typename payload_t>
struct envelope {
  std::string_view sender_name;
  payload_t payload;
};

template <typename... event_ts>
class envelope_generator {
 public:
  [[nodiscard]] explicit envelope_generator(std::string_view sender_name)
      : sender_name_{sender_name} {}

  template <typename payload_t>
  void operator()(const payload_t& payload) {
    if constexpr ((std::is_same_v<payload_t, event_ts> || ...)) {
      enveloped_event_ = envelope<payload_t>{sender_name_, payload};
    }
  }

  [[nodiscard]] const std::variant<envelope<event_ts>...>& get_enveloped_event()
      const noexcept {
    return enveloped_event_;
  }

 private:
  std::string_view sender_name_;
  std::variant<envelope<event_ts>...> enveloped_event_;
};

};  // namespace multithreaded::events
