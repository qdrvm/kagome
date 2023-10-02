/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <type_traits>
#include <vector>

#include <scale/scale.hpp>

#include "common/blob.hpp"
#include "crypto/hasher.hpp"
#include "primitives/common.hpp"
#include "primitives/compact_integer.hpp"
#include "primitives/digest.hpp"
#include "storage/trie/types.hpp"

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
    mutable std::optional<BlockHash> hash_{};  ///< Its block hash if calculated

    bool operator==(const BlockHeader &rhs) const {
      return std::tie(parent_hash, number, state_root, extrinsics_root, digest)
          == std::tie(rhs.parent_hash,
                      rhs.number,
                      rhs.state_root,
                      rhs.extrinsics_root,
                      rhs.digest);
    }

    bool operator!=(const BlockHeader &rhs) const {
      return !operator==(rhs);
    }

    std::optional<primitives::BlockInfo> parentInfo() const {
      if (number != 0) {
        return primitives::BlockInfo{number - 1, parent_hash};
      }
      return std::nullopt;
    }

    const BlockHash &hash(const crypto::Hasher &hasher) const {
      if (not hash_.has_value()) {
        hash_.emplace(hasher.blake2b_256(scale::encode(*this).value()));
      }
      return hash_.value();
    }

    const BlockHash &hash() const {
      BOOST_ASSERT_MSG(hash_.has_value(),
                       "Hash must be calculated and saved before that");
      return hash_.value();
    }

    const BlockHash &block() const {
      BOOST_ASSERT_MSG(hash_.has_value(),
                       "Hash must be calculated and saved before that");
      return hash_.value();
    }
  };

  struct BlockHeaderReflection {
    const BlockHash &parent_hash;
    const BlockNumber &number;
    const storage::trie::RootHash &state_root;
    const common::Hash256 &extrinsics_root;
    gsl::span<const DigestItem> digest;

    BlockHeaderReflection(const BlockHeader &origin)
        : parent_hash(origin.parent_hash),
          number(origin.number),
          state_root(origin.state_root),
          extrinsics_root(origin.extrinsics_root),
          digest(origin.digest) {}
  };

  // Reflection of block header without Seal, which is the last digest
  struct UnsealedBlockHeaderReflection : public BlockHeaderReflection {
    explicit UnsealedBlockHeaderReflection(const BlockHeaderReflection &origin)
        : BlockHeaderReflection(origin) {
      BOOST_ASSERT_MSG(number == 0 or not digest.empty(),
                       "Non-genesis block must have at least Seal digest");
      digest = digest.subspan(0, digest.size() - 1);
    }
    explicit UnsealedBlockHeaderReflection(const BlockHeader &origin)
        : UnsealedBlockHeaderReflection(BlockHeaderReflection(origin)) {}
  };

  struct GenesisBlockHeader {
    const BlockHeader header;
    const BlockHash hash;
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const BlockHeaderReflection &bhr) {
    return s << bhr.parent_hash << CompactInteger(bhr.number) << bhr.state_root
             << bhr.extrinsics_root << bhr.digest;
  }
  /**
   *
   * @brief outputs object of type BlockHeader to stream
   * @tparam Stream output stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const BlockHeader &bh) {
    return s << bh.parent_hash << CompactInteger(bh.number) << bh.state_root
             << bh.extrinsics_root << bh.digest;
  }

  /**
   * @brief decodes object of type BlockHeader from stream
   * @tparam Stream input stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, BlockHeader &bh) {
    CompactInteger number_compact;
    s >> bh.parent_hash >> number_compact >> bh.state_root >> bh.extrinsics_root
        >> bh.digest;
    bh.number = number_compact.convert_to<BlockNumber>();
    return s;
  }

  outcome::result<BlockHash> calculateBlockHash(const BlockHeader &header,
                                                const crypto::Hasher &hasher);

}  // namespace kagome::primitives
