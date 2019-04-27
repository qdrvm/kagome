/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_TESTUTIL_COMMON_HPP
#define KAGOME_TEST_TESTUTIL_COMMON_HPP

#include <fstream>

#include <gtest/gtest.h>
#include "common/buffer.hpp"

namespace test {

  /**
   * Fixture for the tests with wasm to read file and work with it
   */
  class WasmTest : public ::testing::Test {
   public:
    explicit WasmTest(const std::string &path);

   protected:
    kagome::common::Buffer state_code_;
  };
}  // namespace test

#endif  // KAGOME_TEST_TESTUTIL_COMMON_HPP
