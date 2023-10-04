/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_OUTCOME_INTO_HPP
#define KAGOME_OUTCOME_INTO_HPP

#include "outcome/outcome.hpp"

namespace outcome {
  /// Wraps value or returns original result
  template <typename T>
  struct Into {
    static outcome::result<T> into(T &&r) {
      return outcome::success(std::move(r));
    }
  };
  template <typename T>
  struct Into<outcome::result<T>> {
    static outcome::result<T> into(outcome::result<T> &&r) {
      return std::move(r);
    }
  };
  auto into(auto &&r) {
    return Into<std::decay_t<decltype(r)>>::into(std::forward<decltype(r)>(r));
  }
}  // namespace outcome

#endif  // KAGOME_OUTCOME_INTO_HPP
