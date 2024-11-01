/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>

#include <boost/endian/conversion.hpp>

#include "parachain/types.hpp"
#include "primitives/common.hpp"

namespace kagome {
  struct CandidateChunkKey {
    static constexpr size_t kCandidateHashSize =
        sizeof(parachain::CandidateHash);
    static constexpr size_t kChunkIndexSize = sizeof(parachain::ChunkIndex);
    using Key = common::Blob<kCandidateHashSize + kChunkIndexSize>;
    using HashKey = common::Blob<kCandidateHashSize>;

    static Key encode(const parachain::CandidateHash &candidate_hash,
                      const parachain::ChunkIndex &chunk_index) {
      Key key;
      std::copy_n(
          encode_hash(candidate_hash).data(), kCandidateHashSize, key.data());
      boost::endian::store_big_u32(key.data() + kCandidateHashSize,
                                   chunk_index);
      return key;
    }

    static HashKey encode_hash(const parachain::CandidateHash &candidate_hash) {
      HashKey key;
      std::copy_n(candidate_hash.data(), kCandidateHashSize, key.data());
      return key;
    }

    static std::optional<
        std::pair<parachain::CandidateHash, parachain::ChunkIndex>>
    decode(common::BufferView key) {
      if (key.size() != Key::size()) {
        return std::nullopt;
      }
      std::pair<parachain::CandidateHash, parachain::ChunkIndex> candidateChunk;
      std::copy_n(key.data(), kCandidateHashSize, candidateChunk.first.data());
      candidateChunk.second =
          boost::endian::load_big_u32(key.data() + kCandidateHashSize);
      return candidateChunk;
    }
  };
}  // namespace kagome
