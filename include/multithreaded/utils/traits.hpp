#pragma once

#include <variant>
namespace multithreaded::utils {
template <typename T, typename V>
struct variant_holds_alternative;

template <typename T, typename... Us>
struct variant_holds_alternative<T, std::variant<Us...>> {
  static constexpr bool value = (std::is_same_v<T, Us> || ...);
};

template <typename T, typename V>
constexpr bool variant_holds_alternative_v =
    variant_holds_alternative<T, V>::value;
};  // namespace multithreaded::utils
