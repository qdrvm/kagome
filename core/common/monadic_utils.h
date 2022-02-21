/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MONADIC_UTILS_H
#define KAGOME_MONADIC_UTILS_H

#include <optional>
#include <type_traits>

#include "outcome/outcome.hpp"

namespace kagome::common {

  template <typename T,
            typename F,
            typename R = std::invoke_result_t<F, const T &>>
  std::optional<R> map_optional(std::optional<T> const &opt, F const &f) {
    if (opt.has_value()) {
      return std::optional<R>{f(opt.value())};
    }
    return std::nullopt;
  }

  template <typename T, typename F, typename R = std::invoke_result_t<F, T &&>>
  std::optional<R> map_optional(std::optional<T> &&opt, F const &f) {
    if (opt.has_value()) {
      return std::optional<R>{f(std::move(opt.value()))};
    }
    return std::nullopt;
  }

  template <typename T,
            typename F,
            typename R = std::invoke_result_t<F, const T &>>
  outcome::result<R> map_result(
      outcome::result<T> const &res, F const &f) {
    if (res.has_value()) {
      return outcome::result<R>{f(res.value())};
    }
    return res.as_failure();
  }

  template <typename T, typename F, typename R = std::invoke_result_t<F, T &&>>
 outcome::result<R> map_result(
      outcome::result<T> &&res, F const &f) {
    if (res.has_value()) {
      return outcome::result<R>{f(std::move(res.value()))};
    }
    return res.as_failure();
  }

  template <typename T,
            typename F,
            typename R = std::invoke_result_t<F, T const &>>
  outcome::result<std::optional<R>> map_result_optional(
      outcome::result<std::optional<T>> const &res_opt, F const &f) {
    return map_result(res_opt, [&f](auto &opt) {
      return map_optional(opt, [&f](auto &v) { return f(v); });
    });
  }

  template <typename T, typename F, typename R = std::invoke_result_t<F, T &&>>
  outcome::result<std::optional<R>> map_result_optional(
      outcome::result<std::optional<T>> &&res_opt, F const &f) {
    return map_result(res_opt, [&f](auto &opt) {
      return map_optional(opt, [&f](auto &v) { return f(std::move(v)); });
    });
  }

}  // namespace kagome::common

#endif  // KAGOME_MONADIC_UTILS_H
