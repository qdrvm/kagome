/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "testutil/runtime/wasm_test.hpp"

#include <boost/filesystem.hpp>

namespace test {

  WasmTest::WasmTest(const std::string &path) {
    namespace fs = boost::filesystem;
    std::ifstream ifd(path, std::ios::binary | std::ios::ate);
    int size = ifd.tellg();
    ifd.seekg(0, std::ios::beg);
    kagome::common::Buffer b(size, 0);
    ifd.read((char *)b.toBytes(), size);  // NOLINT
    state_code_ = b;
  }

}  // namespace test
