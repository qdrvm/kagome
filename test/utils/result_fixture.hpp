/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RESULT_FIXTURE_HPP
#define KAGOME_RESULT_FIXTURE_HPP

#include "common/result.hpp"

namespace kagome::expected::testing {
  template <typename ResultType>
  using ValueOf = kagome::expected::ValueOf<ResultType>;
  template <typename ResultType>
  using ErrorOf = kagome::expected::ErrorOf<ResultType>;

  /**
   * @return optional with value if present
   *         otherwise none
   */
  template <typename ResultType>
  boost::optional<ValueOf<std::decay_t<ResultType>>> val(
      ResultType &&res) noexcept {
    if (auto *val = boost::get<ValueOf<std::decay_t<ResultType>>>(&res)) {
      return std::move(*val);
    }
    return {};
  }

  /**
   * @return optional with error if present
   *         otherwise none
   */
  template <typename ResultType>
  boost::optional<ErrorOf<std::decay_t<ResultType>>> err(
      ResultType &&res) noexcept {
    if (auto *val = boost::get<ErrorOf<std::decay_t<ResultType>>>(&res)) {
      return std::move(*val);
    }
    return {};
  }
}  // namespace kagome::expected::testing

#endif  // KAGOME_RESULT_FIXTURE_HPP
