/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "runtime/impl/basic_wasm_provider.hpp"

#include <fstream>

namespace test {
  using kagome::common::Buffer;

  BasicWasmProvider::BasicWasmProvider(std::string_view path) {
    initialize(path);
  }

  const Buffer &BasicWasmProvider::getStateCode() const {
    return buffer_;
  }

  void BasicWasmProvider::initialize(std::string_view path) {
    // std::ios::ate seeks to the end of file
    std::ifstream ifd(std::string(path), std::ios::binary | std::ios::ate);
    // so size means count of bytes in file
    int size = ifd.tellg();
    // set position to the beginning
    ifd.seekg(0, std::ios::beg);
    kagome::common::Buffer buffer(size, 0);
    // read whole file to the buffer
    ifd.read((char *)buffer.data(), size);  // NOLINT
    buffer_ = std::move(buffer);
  }
}  // namespace test
