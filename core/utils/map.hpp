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

  template <typename C>
  inline std::optional<typename C::const_iterator> get(
      const C &container, const typename C::key_type &key) {
    if (auto it = container.find(key); it != container.end()) {
      return it;
    }
    return std::nullopt;
  }

  template <typename C>
  inline std::optional<typename C::iterator> get(
      C &container, const typename C::key_type &key) {
    if (auto it = container.find(key); it != container.end()) {
      return it;
    }
    return std::nullopt;
  }

  template <typename T>
  inline auto get(const std::vector<T> &container, const size_t &index) {
    using ItT = std::vector<T>::const_iterator;
    if (index >= container.size()) {
      return std::optional<ItT>{};
    }
    auto it = container.begin();
    std::advance(it, index);
    return std::make_optional(it);
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
