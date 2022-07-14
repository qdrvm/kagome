/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_TESTUTIL_PRIMITIVES_MP_UTILS_HPP
#define KAGOME_TEST_TESTUTIL_PRIMITIVES_MP_UTILS_HPP

#include "common/blob.hpp"

namespace testutil {
  /**
   * @brief creates hash from initializers list
   * @param bytes initializers
   * @return newly created hash
   */
  kagome::common::Hash256 createHash256(std::initializer_list<uint8_t> bytes);

}  // namespace testutil

#endif  // KAGOME_TEST_TESTUTIL_PRIMITIVES_MP_UTILS_HPP
