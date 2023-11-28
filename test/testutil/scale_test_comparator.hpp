/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_SCALE_TEST_COMPARATOR_HPP
#define KAGOME_SCALE_TEST_COMPARATOR_HPP

#include "scale/kagome_scale.hpp"
#include "scale/scale.hpp"

namespace testutil {

  template <typename... T>
  inline outcome::result<std::vector<uint8_t>> scaleEncodeAndCompareWithRef(
      T &&...t) {
    return ::scale::encode(std::forward<T>(t)...);
  }

}  // namespace testutil

#endif  // KAGOME_SCALE_TEST_COMPARATOR_HPP
