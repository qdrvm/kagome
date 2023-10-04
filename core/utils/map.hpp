/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_UTILS_MAP_HPP
#define KAGOME_UTILS_MAP_HPP

#include <functional>
#include <optional>
#include <type_traits>

namespace kagome::utils {

  template <typename T, typename F>
  inline std::optional<
      typename std::result_of_t<F(typename std::decay_t<T>::value_type &&)>>
  map(T &&source, F &&func) {
    if (source) {
      return {std::forward<F>(func)(*std::forward<T>(source))};
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
  };

}  // namespace kagome::utils

#endif  // KAGOME_UTILS_MAP_HPP