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
  struct container {  // NOLINT
    virtual ~container() = default;
  };

  template <typename... event_ts>
  class model : public container {
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

  std::unique_ptr<container> pimpl_;

 public:
  template <typename... event_ts>
  explicit connector(
      [[maybe_unused]] connector_event_set<event_ts...> event_set)
      : pimpl_{std::make_unique<model<event_ts...>>()} {}

  connector(const connector&) = delete;
  connector& operator=(const connector&) = delete;

  connector(connector&&) = default;
  connector& operator=(connector&&) = default;

  ~connector() = default;

  template <typename... event_ts>
  [[nodiscard]] model<event_ts...>& as_connector_of() const {
    auto* model_ptr = dynamic_cast<model<event_ts...>*>(pimpl_.get());
    if (model_ptr == nullptr) {
      // TODO: Consider a better way to handle error instead of throwing
      throw std::bad_cast{};
    }
    return *model_ptr;
  }
};

}  // namespace multithreaded
