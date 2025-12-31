#pragma once

#include <concepts>
#include <functional>
#include <typeinfo>
#include <variant>

#include "utils/queue.hpp"
namespace multithreaded {

template <typename... event_ts>
struct connector_event_set {};

template <typename... event_ts>
class read_only_port;

template <typename... event_ts>
class write_only_port;

class any_read_port {
 private:
  struct port_concept {
    virtual ~port_concept() = default;
  };

  template <typename... event_ts>
  class port_model : public port_concept {
   public:
    port_model(read_only_port<event_ts...>&& rport)
        : read_port_{std::move(rport)} {}

    template <typename queue_elem_t>
    [[nodiscard]] bool pop(queue_elem_t& elem) noexcept {
      if constexpr ((std::is_same_v<event_ts, queue_elem_t> || ...)) {
        return read_port_.pop(elem);
      }
      return false;
    }

   private:
    read_only_port<event_ts...> read_port_;
  };

  std::unique_ptr<port_concept> pimpl_;

 public:
  template <typename... event_ts>
  any_read_port(read_only_port<event_ts...>&& rport)
      : pimpl_{std::make_unique<port_model<event_ts...>>(std::move(rport))} {}

  // template <typename... event_ts>
  // [[nodiscard]] port_model<event_ts...>& as_port_of() const {
  //   auto* model_ptr = dynamic_cast<port_model<event_ts...>*>(pimpl_.get());
  //   if (!model_ptr) {
  //     throw std::bad_cast{};
  //   }
  //   return *model_ptr;
  // }
};

class any_write_port {
 private:
  struct port_concept {
    virtual ~port_concept() = default;
  };

  template <typename... event_ts>
  class port_model : public port_concept {
   public:
    port_model(write_only_port<event_ts...>&& wport)
        : write_port_{std::move(wport)} {}

    template <typename queue_elem_t>
    [[nodiscard]] bool push(const queue_elem_t& elem) noexcept {
      if constexpr ((std::is_same_v<event_ts, queue_elem_t> || ...)) {
        return write_port_.push(elem);
      }
      return false;
    }

    template <typename queue_elem_t, typename... arg_ts>
      requires std::constructible_from<queue_elem_t, arg_ts...>
    [[nodiscard]] bool emplace(arg_ts&&... args) noexcept {
      if constexpr ((std::is_same_v<event_ts, queue_elem_t> || ...)) {
        return write_port_.emplace(std::forward<event_ts>(args)...);
      }
      return false;
    }

   private:
    write_only_port<event_ts...> write_port_;
  };

  std::unique_ptr<port_concept> pimpl_;

 public:
  template <typename... event_ts>
  any_write_port(write_only_port<event_ts...>&& wport)
      : pimpl_{std::make_unique<port_model<event_ts...>>(std::move(wport))} {}
};

template <typename... event_ts>
class read_only_port {
 private:
  using queue_t = utils::spsc_queue<std::variant<event_ts...>>;
  using queue_elem_t = typename queue_t::elem_t;

 public:
  read_only_port(const std::reference_wrapper<queue_t>& queue_ref)
      : queue_{queue_ref} {}

  [[nodiscard]] bool pop(queue_elem_t& elem) noexcept {
    return queue_.get().pop(elem);
  }

 private:
  std::reference_wrapper<queue_t> queue_;
};

template <typename... event_ts>
class write_only_port {
 private:
  using queue_t = utils::spsc_queue<std::variant<event_ts...>>;
  using queue_elem_t = typename queue_t::elem_t;

 public:
  [[nodiscard]] bool push(const queue_elem_t& elem) noexcept {
    return queue_.get().push(elem);
  }

  template <typename... arg_ts>
    requires std::constructible_from<queue_elem_t, arg_ts...>
  [[nodiscard]] bool emplace(arg_ts&&... args) noexcept {
    return queue_.get().emplace(std::forward<arg_ts>(args)...);
  }

  write_only_port(const std::reference_wrapper<queue_t>& queue_ref)
      : queue_{queue_ref} {}

 private:
  std::reference_wrapper<queue_t> queue_;
};

class connector {
 private:
  struct connector_concept {
    virtual ~connector_concept() = default;
  };

  template <typename... event_ts>
  class connector_model : public connector_concept {
    using queue_t = utils::spsc_queue<std::variant<event_ts...>>;

   public:
    [[nodiscard]] read_only_port<event_ts...> get_read_port() noexcept {
      return read_only_port<event_ts...>{std::ref(queue_)};
    }

    [[nodiscard]] write_only_port<event_ts...> get_write_port() noexcept {
      return write_only_port<event_ts...>{std::ref(queue_)};
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
