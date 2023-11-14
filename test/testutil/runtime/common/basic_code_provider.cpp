/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "testutil/runtime/common/basic_code_provider.hpp"

#include "utils/read_file.hpp"

namespace kagome::runtime {
  using kagome::common::Buffer;

  BasicCodeProvider::BasicCodeProvider(std::string_view path) {
    initialize(path);
  }

  outcome::result<common::BufferView> BasicCodeProvider::getCodeAt(
      const storage::trie::RootHash &at) const {
    return buffer_;
  }

  void BasicCodeProvider::initialize(std::string_view path) {
    if (not readFile(buffer_, std::string{path})) {
      throw std::runtime_error("File with test code " + std::string(path)
                               + " not found");
    }
  }
}  // namespace kagome::runtime
