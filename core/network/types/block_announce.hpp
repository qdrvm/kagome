/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BLOCK_ANNOUNCE_HPP
#define KAGOME_BLOCK_ANNOUNCE_HPP

#include "primitives/block_header.hpp"
#include "scale/scale.hpp"

namespace kagome::network {

  /// Block state in the chain.
  enum class BlockState : uint8_t {
    /// Block is not part of the best chain.
    Normal,
    /// Latest best block.
    Best,
  };

  /// Announce a new complete relay chain block on the network.
  struct BlockAnnounce {
    /// New block header.
    primitives::BlockHeader header;

    /// Block state.
    std::optional<BlockState> state = std::nullopt;

    /// Data associated with this block announcement, e.g. a candidate message.
    std::optional<std::vector<uint8_t>> data = std::nullopt;

    // operator== is needed only for tests
    inline bool operator==(const BlockAnnounce &other) const {
      return this->header == other.header and this->state == other.state
         and this->data == other.data;
    }

    friend inline scale::ScaleEncoderStream &operator<<(
        scale::ScaleEncoderStream &s, const BlockAnnounce &v) {
      s << v.header;
      if (v.state.has_value()) {
        s << v.state.value();
        if (v.data.has_value()) {
          s << v.data.value();
        }
      }
      return s;
    }

    friend inline scale::ScaleDecoderStream &operator>>(
        scale::ScaleDecoderStream &s, BlockAnnounce &v) {
      s >> v.header;
      if (s.hasMore(1)) {
        BlockState tmp;
        s >> tmp;
        v.state.emplace(tmp);
      } else {
        v.state.reset();
      }
      if (s.hasMore(1)) {
        std::vector<uint8_t> tmp;
        s >> tmp;
        v.data.emplace(std::move(tmp));
      } else {
        v.data.reset();
      }
      return s;
    }
  };

}  // namespace kagome::network

#endif  // KAGOME_BLOCK_ANNOUNCE_HPP
