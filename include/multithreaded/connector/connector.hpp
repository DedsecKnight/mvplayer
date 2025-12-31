#pragma once

#include <functional>
#include <typeinfo>
#include <variant>

#include "connector/read/port.hpp"
#include "connector/write/port.hpp"
#include "utils/queue.hpp"

namespace multithreaded {

template <typename... event_ts>
struct connector_event_set {};

class connector {
 private:
  struct connector_concept {
    virtual ~connector_concept() = default;
  };

  template <typename... event_ts>
  class connector_model : public connector_concept {
    using queue_t = utils::spsc_queue<std::variant<event_ts...>>;

   public:
    [[nodiscard]] read::port<event_ts...> get_read_port() noexcept {
      return read::port<event_ts...>{std::ref(queue_)};
    }

    [[nodiscard]] write::port<event_ts...> get_write_port() noexcept {
      return write::port<event_ts...>{std::ref(queue_)};
    }

   private:
    queue_t queue_;
  };

  std::unique_ptr<connector_concept> pimpl_;

 public:
  template <typename... event_ts>
  connector(connector_event_set<event_ts...>&&)
      : pimpl_{std::make_unique<connector_model<event_ts...>>()} {}

  template <typename... event_ts>
  connector_model<event_ts...>& as_connector_of() const {
    auto* model_ptr = dynamic_cast<connector_model<event_ts...>*>(pimpl_.get());
    if (model_ptr == nullptr) {
      // TODO: Consider a better way to handle error instead of throwing
      throw std::bad_cast{};
    }
    return *model_ptr;
  }
};

}  // namespace multithreaded
