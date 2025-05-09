/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "testutil/runtime/common/basic_code_provider.hpp"

#include "utils/read_file.hpp"

namespace kagome::runtime {
  BasicCodeProvider::BasicCodeProvider(std::string_view path) {
    initialize(path);
  }

  RuntimeCodeProvider::Result BasicCodeProvider::getCodeAt(
      const storage::trie::RootHash &at) const {
    return buffer_;
  }

  void BasicCodeProvider::initialize(std::string_view path) {
    common::Buffer code;
    if (not readFile(code, std::string{path})) {
      throw std::runtime_error("File with test code " + std::string(path)
                               + " not found");
    }
    buffer_ = std::make_shared<common::Buffer>(std::move(code));
  }
}  // namespace kagome::runtime
