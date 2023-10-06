/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gsl/span>
#include <optional>

#include "primitives/block_id.hpp"
#include "storage/trie/types.hpp"

namespace kagome::runtime {
  /**
   * @class RuntimeCodeProvider keeps and provides wasm state code
   */
  class RuntimeCodeProvider {
   public:
    virtual ~RuntimeCodeProvider() = default;

    virtual outcome::result<gsl::span<const uint8_t>> getCodeAt(
        const storage::trie::RootHash &state) const = 0;
  };

}  // namespace kagome::runtime
