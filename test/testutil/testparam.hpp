/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_TESTUTIL_TESTPARAM_HPP
#define KAGOME_TEST_TESTUTIL_TESTPARAM_HPP

#include <cstdint>
#include <vector>

namespace testutil {
  template <typename T>
  struct TestParam {
    std::vector<uint8_t> encoded_value{};
    bool should_fail{false};
    T value{};
  };

  template <typename T>
  TestParam<T> make_param(std::vector<uint8_t> &&buffer,
                          bool should_fail,
                          T &&value) {
    return {buffer, should_fail, std::forward<T>(value)};
  };
}  // namespace testutil

#endif  // KAGOME_TEST_TESTUTIL_TESTPARAM_HPP
