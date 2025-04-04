/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <type_traits>
#include <vector>

#include <scale/scale.hpp>

#include "common/blob.hpp"
#include "crypto/hasher.hpp"
#include "primitives/common.hpp"
#include "primitives/digest.hpp"
#include "storage/trie/types.hpp"

#include <common/custom_equality.hpp>

namespace kagome::primitives {
  /**
   * @struct BlockHeader represents header of a block
   */
  struct BlockHeader {
    BlockNumber number{};                  ///< Block number (height)
    BlockHash parent_hash{};               ///< Parent block hash
    storage::trie::RootHash state_root{};  ///< Merkle tree root of state
    common::Hash256 extrinsics_root{};     ///< Hash of included extrinsics
    Digest digest{};                       ///< Chain-specific auxiliary data
    mutable std::optional<BlockHash> hash_opt{};  ///< Block hash if calculated

    CUSTOM_EQUALITY(BlockHeader,  //
                    parent_hash,
                    number,
                    state_root,
                    extrinsics_root,
                    digest)
    SCALE_CUSTOM_DECOMPOSITION(BlockHeader,
                               parent_hash,
                               scale::as_compact(number),
                               state_root,
                               extrinsics_root,
                               digest)

    std::optional<primitives::BlockInfo> parentInfo() const {
      if (number != 0) {
        return primitives::BlockInfo{number - 1, parent_hash};
      }
      return std::nullopt;
    }

    const BlockHash &hash() const {
      BOOST_ASSERT_MSG(hash_opt.has_value(),
                       "Hash must be calculated and saved before that");
      return hash_opt.value();
    }

    void updateHash(const crypto::Hasher &hasher) const {
      auto enc_res = scale::encode(*this);
      BOOST_ASSERT_MSG(enc_res.has_value(),
                       "Header should be encoded errorless");
      hash_opt.emplace(hasher.blake2b_256(enc_res.value()));
    }

    BlockInfo blockInfo() const {
      return {number, hash()};
    }
  };

  struct BlockHeaderReflection {
    // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
    const BlockHash &parent_hash;
    const BlockNumber &number;
    const storage::trie::RootHash &state_root;
    const common::Hash256 &extrinsics_root;
    std::span<const DigestItem> digest;
    // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)

    BlockHeaderReflection(const BlockHeader &origin)
        : parent_hash(origin.parent_hash),
          number(origin.number),
          state_root(origin.state_root),
          extrinsics_root(origin.extrinsics_root),
          digest(origin.digest) {}
    SCALE_CUSTOM_DECOMPOSITION(BlockHeaderReflection,
                               parent_hash,
                               scale::as_compact(number),
                               state_root,
                               extrinsics_root,
                               digest);
  };

  // Reflection of block header without Seal, which is the last digest
  struct UnsealedBlockHeaderReflection : public BlockHeaderReflection {
    explicit UnsealedBlockHeaderReflection(const BlockHeaderReflection &origin)
        : BlockHeaderReflection(origin) {
      BOOST_ASSERT_MSG(number == 0 or not digest.empty(),
                       "Non-genesis block must have at least Seal digest");
      digest = digest.first(digest.size() - 1);
    }
    explicit UnsealedBlockHeaderReflection(const BlockHeader &origin)
        : UnsealedBlockHeaderReflection(BlockHeaderReflection(origin)) {}
  };

  struct GenesisBlockHeader {
    // NOLINTBEGIN(cppcoreguidelines-avoid-const-or-ref-data-members)
    const BlockHeader header;
    const BlockHash hash;
    // NOLINTEND(cppcoreguidelines-avoid-const-or-ref-data-members)
  };

  void calculateBlockHash(const BlockHeader &header,
                          const crypto::Hasher &hasher);

}  // namespace kagome::primitives
