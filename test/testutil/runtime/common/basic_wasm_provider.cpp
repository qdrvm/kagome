/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "testutil/runtime/common/basic_wasm_provider.hpp"

#include <fstream>

namespace kagome::runtime {
  using kagome::common::Buffer;

  BasicCodeProvider::BasicCodeProvider(std::string_view path) {
    initialize(path);
  }

  outcome::result<gsl::span<const uint8_t>> BasicCodeProvider::getCodeAt(
      const storage::trie::RootHash &at) const {
    return buffer_;
  }


  void BasicCodeProvider::initialize(std::string_view path) {
    // std::ios::ate seeks to the end of file
    std::ifstream ifd(std::string(path), std::ios::binary | std::ios::ate);
    // so size means count of bytes in file
    int size = ifd.tellg();
    // set position to the beginning
    ifd.seekg(0, std::ios::beg);
    kagome::common::Buffer buffer(size, 0);
    // read whole file to the buffer
    ifd.read(reinterpret_cast<char *>(buffer.data()), size);  // NOLINT
    buffer_ = std::move(buffer);
  }
}  // namespace kagome::runtime
