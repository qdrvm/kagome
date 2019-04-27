/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "testutil/common.hpp"

namespace test {

  kagome::common::Buffer readFile(const std::string &path) {
    namespace fs = boost::filesystem;
    std::ifstream ifd(path, std::ios::binary | std::ios::ate);
    int size = ifd.tellg();
    ifd.seekg(0, std::ios::beg);
    kagome::common::Buffer b(size, 0);
    ifd.read((char *)b.toBytes(), size);
    return b;
  }

}  // namespace test
