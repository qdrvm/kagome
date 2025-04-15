/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/buffer.hpp"
#include "storage/trie/types.hpp"

#include <memory>

namespace kagome::runtime {
  /**
   * @class RuntimeCodeProvider keeps and provides wasm state code
   */
  class RuntimeCodeProvider {
   public:
    virtual ~RuntimeCodeProvider() = default;

    using Code = std::shared_ptr<const common::Buffer>;
    using Result = outcome::result<Code>;

    virtual Result getCodeAt(const storage::trie::RootHash &state) const = 0;
    virtual Result getPendingCodeAt(
        const storage::trie::RootHash &state) const = 0;
  };

}  // namespace kagome::runtime
