/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>

#include "primitives/block.hpp"
#include "primitives/justification.hpp"

namespace kagome::primitives {

  /**
   * Data, describing the block. Used for example in BlockRequest, where we need
   * to get certain information about the block
   */
  struct BlockData {
    primitives::BlockHash hash;
    std::optional<primitives::BlockHeader> header{};
    std::optional<primitives::BlockBody> body{};
    std::optional<common::Buffer> receipt{};
    std::optional<common::Buffer> message_queue{};
    std::optional<primitives::Justification> justification{};
    std::optional<primitives::Justification> beefy_justification{};
    bool operator==(const BlockData &other) const = default;
  };

  struct BlockDataFlags {
    static BlockDataFlags allSet(primitives::BlockHash hash) {
      // NOLINTNEXTLINE(modernize-use-designated-initializers)
      return BlockDataFlags{hash, true, true, true, true, true};
    }

    static BlockDataFlags allUnset(primitives::BlockHash hash) {
      // NOLINTNEXTLINE(modernize-use-designated-initializers)
      return BlockDataFlags{hash, false, false, false, false, false};
    }

    primitives::BlockHash hash;
    bool header{};
    bool body{};
    bool receipt{};
    bool message_queue{};
    bool justification{};
  };
}  // namespace kagome::primitives
