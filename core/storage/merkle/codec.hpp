/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_MERKLE_UTIL_HPP
#define KAGOME_MERKLE_UTIL_HPP

#include "common/blob.hpp"  // for Hash256
#include "common/buffer.hpp"

#include "storage/merkle/node.hpp"

namespace kagome::storage::merkle {

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
    virtual outcome::result<common::Buffer> encodeNode(
        const Node &node) const = 0;

    /**
     * @brief Decode node from bytes
     * @param encoded_data a buffer containing encoded representation of a node
     * @return a node in the trie
     */
    virtual outcome::result<std::shared_ptr<Node>> decodeNode(
        const common::Buffer &encoded_data) const = 0;

    /**
     * @brief Algorithm that is used for hashing of nodes.
     * @param buf node's byte representation
     * @return hash of the node.
     */
    virtual common::Hash256 hash256(const common::Buffer &buf) const = 0;
  };

}  // namespace kagome::storage::merkle

#endif  // KAGOME_MERKLE_UTIL_HPP
