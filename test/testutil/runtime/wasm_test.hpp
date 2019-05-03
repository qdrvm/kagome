/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_TESTUTIL_COMMON_HPP
#define KAGOME_TEST_TESTUTIL_COMMON_HPP

#include <fstream>
#include <memory>

#include <gtest/gtest.h>
#include <boost/filesystem/path.hpp>
#include "primitives/block.hpp"
#include "primitives/block_header.hpp"
#include "core/storage/merkle/mock_trie_db.hpp"
#include "extensions/extension_impl.hpp"
#include "primitives/impl/scale_codec_impl.hpp"
#include "runtime/impl/wasm_memory_impl.hpp"
#include "testutil/outcome.hpp"

namespace test {

  /**
   * Fixture for wasm tests that read a wasm code file
   */
  class WasmTest: public ::testing::Test {
   public:
      explicit WasmTest(const std::string& filepath);

  protected:
    kagome::common::Buffer state_code_;

  };
}  // namespace test

#endif  // KAGOME_TEST_TESTUTIL_COMMON_HPP
