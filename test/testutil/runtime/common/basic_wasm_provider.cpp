/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "testutil/runtime/common/basic_wasm_provider.hpp"

#include <fstream>

namespace kagome::runtime {
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
}  // namespace kagome::runtime
