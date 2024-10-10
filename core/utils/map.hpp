/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_UTILS_MAP_HPP
#define KAGOME_UTILS_MAP_HPP

#include <functional>
#include <optional>
#include <type_traits>

#include <boost/utility/result_of.hpp>

namespace kagome::utils {

  template <typename T, typename F>
  inline std::optional<typename boost::result_of<
      F(typename std::decay_t<T>::value_type &&)>::type>
  map(T &&source, F &&func) {
    if (source) {
      return {std::forward<F>(func)(*std::forward<T>(source))};
    }
    return std::nullopt;
  }

  template <typename C> requires requires { typename C::mapped_type; }
  inline std::optional<std::reference_wrapper<const typename C::mapped_type>> get(
      const C &container, const typename C::key_type &key) {
      if (auto it = container.find(key); it != container.end()) {
        return {{it->second}};
      }
      return std::nullopt;
  }

  template <typename C> requires requires { typename C::mapped_type; }
  inline std::optional<std::reference_wrapper<typename C::mapped_type>> get(
      C &container, const typename C::key_type &key) {
      if (auto it = container.find(key); it != container.end()) {
        return {{it->second}};
      }
      return std::nullopt;
  }

template <typename C> requires requires { typename C::value_type; std::is_same_v<is_pair<typename C::value_type>, std::false_type>; }
inline std::optional<std::reference_wrapper<const typename C::value_type>> get(
    const C &container, const size_t &index) {
    if (index < container.size()) {
        return {{container[index]}};
    }
    return std::nullopt;
}

template <typename C> requires requires { typename C::value_type; std::is_same_v<is_pair<typename C::value_type>, std::false_type>; }
inline std::optional<std::reference_wrapper<typename C::value_type>> get(
    C &container, const size_t &index) {
    if (index < container.size()) {
        return {{container[index]}};
    }
    return std::nullopt;
}

  template <typename T>
  inline auto fromRefToOwn(
      const std::optional<std::reference_wrapper<T>> &opt_ref) {
    std::optional<std::decay_t<T>> val{};
    if (opt_ref) {
      val = opt_ref->get();
    }
    return val;
  }

}  // namespace kagome::utils

#endif  // KAGOME_UTILS_MAP_HPP
