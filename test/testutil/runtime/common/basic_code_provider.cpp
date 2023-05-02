/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "testutil/runtime/common/basic_code_provider.hpp"

#include "utils/read_file.hpp"

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
    common::Buffer buffer;
    if (not readFile(buffer_, std::string{path})) {
      throw std::runtime_error("File with test code " + std::string(path)
                               + " not found");
    }
  }
}  // namespace kagome::runtime
