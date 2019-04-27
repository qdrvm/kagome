/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_TESTUTIL_COMMON_HPP
#define KAGOME_TEST_TESTUTIL_COMMON_HPP

#include <fstream>

#include <boost/filesystem.hpp>
#include "common/buffer.hpp"

namespace test {

  /**
   * Read file into the buffer
   */
  kagome::common::Buffer readFile(const std::string &path);
}  // namespace test

#endif  // KAGOME_TEST_TESTUTIL_COMMON_HPP
