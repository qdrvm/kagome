/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRIE_CODEC_HPP
#define KAGOME_TRIE_CODEC_HPP

#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "storage/trie/node.hpp"

namespace kagome::storage::trie {
  struct StoreChildren {
    virtual ~StoreChildren() = default;
    virtual outcome::result<void> store(const common::BufferView &hash,
                                        common::Buffer &&encoded) {
      return outcome::success();
    }
  };

  /**
   * @brief Internal codec for nodes in the Trie. Eth and substrate have
   * different codecs, but rest of the code should be same.
   */
  class Codec {
   public:
    virtual ~Codec() = default;

    /**
     * @brief Encode node to byte representation
     * @param node node in the trie
     * @return encoded representation of a {@param node}
     */
    virtual outcome::result<common::Buffer> encodeNodeAndStoreChildren(
        const Node &node, StoreChildren &store_children) const = 0;

    outcome::result<common::Buffer> encodeNode(const Node &node) const {
      StoreChildren store_children;
      return encodeNodeAndStoreChildren(node, store_children);
    }

    /**
     * @brief Decode node from bytes
     * @param encoded_data a buffer containing encoded representation of a node
     * @return a node in the trie
     */
    virtual outcome::result<std::shared_ptr<Node>> decodeNode(
        gsl::span<const uint8_t> encoded_data) const = 0;

    /**
     * @brief Get the merkle value of a node
     * @param buf byte representation of the node
     * @return hash of \param buf or \param buf if it is shorter than the hash
     */
    virtual common::Buffer merkleValue(const common::BufferView &buf) const = 0;

    virtual bool isMerkleHash(const common::BufferView &buf) const = 0;

    /**
     * @brief Get the hash of a node
     * @param buf byte representation of the node
     * @return hash of \param buf
     */
    virtual common::Hash256 hash256(const common::BufferView &buf) const = 0;
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_TRIE_CODEC_HPP
