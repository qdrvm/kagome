/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>
#include <span>

#include "common/buffer_view.hpp"
#include "primitives/block_id.hpp"
#include "storage/trie/types.hpp"

namespace kagome::runtime {
  /**
   * @class RuntimeCodeProvider keeps and provides wasm state code
   */
  class RuntimeCodeProvider {
   public:
    virtual ~RuntimeCodeProvider() = default;

    virtual outcome::result<common::BufferView> getCodeAt(
        const storage::trie::RootHash &state) const = 0;
  };

}  // namespace kagome::runtime
