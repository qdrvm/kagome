/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_PRIMITIVES_BLOCK_DATA_HPP
#define KAGOME_CORE_PRIMITIVES_BLOCK_DATA_HPP

#include <optional>

#include "primitives/block.hpp"
#include "primitives/justification.hpp"
#include "scale/tie.hpp"

namespace kagome::primitives {

  /**
   * Data, describing the block. Used for example in BlockRequest, where we need
   * to get certain information about the block
   */
  struct BlockData {
    SCALE_TIE(6);

    primitives::BlockHash hash;
    std::optional<primitives::BlockHeader> header{};
    std::optional<primitives::BlockBody> body{};
    std::optional<common::Buffer> receipt{};
    std::optional<common::Buffer> message_queue{};
    std::optional<primitives::Justification> justification{};
  };

  struct BlockDataFlags {
    static BlockDataFlags allSet(primitives::BlockHash hash) {
      return BlockDataFlags{std::move(hash), true, true, true, true, true};
    }

    static BlockDataFlags allUnset(primitives::BlockHash hash) {
      return BlockDataFlags{std::move(hash), false, false, false, false, false};
    }

    primitives::BlockHash hash;
    bool header{};
    bool body{};
    bool receipt{};
    bool message_queue{};
    bool justification{};
  };

  /// Context of processing block, to avoid additional obtaining data from
  /// storage, or redundant calculation. Contains the same data like BlockData,
  /// but by reference, not by value.
  class BlockContext {
    template <typename T>
    using OptConstRef = std::optional<std::reference_wrapper<const T>>;

   public:
    primitives::BlockInfo block;
    OptConstRef<primitives::BlockHeader> header{};
    OptConstRef<primitives::BlockBody> body{};
    OptConstRef<common::Buffer> receipt{};
    OptConstRef<common::Buffer> message_queue{};
    OptConstRef<primitives::Justification> justification{};

    bool operator<(const BlockContext &other) const noexcept {
      return block < other.block;
    }

    bool operator==(const BlockContext &other) const noexcept {
      return block == other.block;
    }
  };

}  // namespace kagome::primitives

#endif  // KAGOME_CORE_PRIMITIVES_BLOCK_DATA_HPP
