/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "testutil/runtime/wasm_test.hpp"

#include <string_view>

#include <boost/filesystem.hpp>

using kagome::common::Buffer;

namespace test {

  WasmTest::WasmTest(const std::string& filename) {
    std::ifstream ifd(filename, std::ios::binary | std::ios::ate);
    int size = ifd.tellg();
    ifd.seekg(0, std::ios::beg);
    Buffer b(size, 0);
    ifd.read((char *)b.toBytes(), size);
    state_code_ = b;

  }

}  // namespace test
