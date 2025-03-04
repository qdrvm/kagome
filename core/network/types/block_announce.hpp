/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/block_header.hpp"
#include "scale/kagome_scale.hpp"

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

    friend inline void encode(const BlockAnnounce &v, scale::Encoder &encoder) {
      encode(v.header, encoder);
      if (v.state.has_value()) {
        encode(v.state.value(), encoder);
        if (v.data.has_value()) {
          encode(v.data.value(), encoder);
        }
      }
    }

    friend inline void decode(BlockAnnounce &v, scale::Decoder &decoder) {
      decode(v.header, decoder);
      if (decoder.has(1)) {
        decode(v.state.emplace(), decoder);
        if (decoder.has(1)) {
          decode(v.data.emplace(), decoder);
        } else {
          v.data.reset();
        }
      } else {
        v.state.reset();
        v.data.reset();
      }
    }
  };

}  // namespace kagome::network
