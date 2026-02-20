
#pragma once

#include <typeindex>

#include "connector/write/event_port.hpp"
#include "port.hpp"

namespace multithreaded::write {
class any {
 private:
  class container {  // NOLINT
   public:
    virtual ~container() = default;

    template <typename event_t>
    [[nodiscard]] constexpr bool send_event(const event_t& event) noexcept {
      namespace ranges = std::ranges;
      std::type_index type_id{typeid(event_t)};
      if (ranges::find(invalid_types_, type_id) != std::cend(invalid_types_)) {
        return false;
      }

      const auto port_it = ranges::find_if(
          casted_port_mapper_,
          [&type_id](const auto& port) { return port.first == type_id; });
      bool port_cached = port_it != std::cend(casted_port_mapper_);
      auto event_port_ptr =
          port_cached ? port_it->second
                            ->template as_event_port_of<event_port<event_t>>()
                      : dynamic_cast<event_port<event_t>*>(this);
      if (!event_port_ptr) {
        invalid_types_.push_back(type_id);
        return false;
      }
      if (!port_cached) {
        casted_port_mapper_.emplace_back(type_id, event_port_ptr);
      }
      return event_port_ptr->send_event(event);
    }

   private:
    std::vector<std::pair<std::type_index, event_port_wrapper*>>
        casted_port_mapper_;
    std::vector<std::type_index> invalid_types_;
  };

  template <typename... event_ts>
  class model : public container,
                public event_port_impl<model<event_ts...>, event_ts>... {
   public:
    explicit model(port<event_ts...>&& wport) : write_port_{std::move(wport)} {}

    template <typename event_t>
    [[nodiscard]] bool push(const event_t& elem) noexcept {
      if constexpr ((std::is_same_v<event_ts, event_t> || ...)) {
        return write_port_.push(elem);
      }
      return false;
    }

    template <typename queue_elem_t, typename... arg_ts>
      requires std::constructible_from<queue_elem_t, arg_ts...>
    [[nodiscard]] bool emplace(arg_ts&&... args) noexcept {
      if constexpr ((std::is_same_v<event_ts, queue_elem_t> || ...)) {
        return write_port_.emplace(std::forward<arg_ts>(args)...);
      }
      return false;
    }

   private:
    port<event_ts...> write_port_;
  };

  std::unique_ptr<container> pimpl_;

 public:
  template <typename... event_ts>
  explicit any(port<event_ts...>&& wport)
      : pimpl_{std::make_unique<model<event_ts...>>(std::move(wport))} {}

  template <typename event_t>
  [[nodiscard]] bool send_event(const event_t& event) noexcept {
    return pimpl_->send_event(event);
  }
};
}  // namespace multithreaded::write
