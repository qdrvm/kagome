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
    std::vector<uint8_t> data_0;
    kagome::scale::encode(
        [&](const uint8_t *const val, size_t count) {
          for (size_t i = 0; i < count; ++i) {
            data_0.emplace_back(val[i]);
          }
        },
        t...);

    std::vector<uint8_t> data_1 = ::scale::encode(t...).value();
    assert(data_0.size() == data_1.size());
    for (size_t ix = 0; ix < data_0.size(); ++ix) {
      assert(data_0[ix] == data_1[ix]);
    }

    return outcome::success(data_0);
  }

}  // namespace testutil

#endif  // KAGOME_SCALE_TEST_COMPARATOR_HPP
