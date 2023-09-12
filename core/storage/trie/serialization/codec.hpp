/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRIE_CODEC_HPP
#define KAGOME_TRIE_CODEC_HPP

#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "storage/trie/polkadot_trie/trie_node.hpp"
#include "storage/trie/types.hpp"

namespace kagome::storage::trie {
  struct TrieNode;

  /**
   * @brief Internal codec for nodes in the Trie.
   */
  class Codec {
   public:
    struct ChildData {
      const TrieNode &child;
      const MerkleValue &merkle_value;
      common::Buffer encoding;
    };
    struct ValueData {
      const TrieNode &node;
      common::Hash256 hash;
      const common::Buffer &value;
    };
    using Visitee = std::variant<ChildData, ValueData>;
    using ChildVisitor = std::function<outcome::result<void>(Visitee)>;

    static constexpr auto NoopChildVisitor =
        [](Visitee) -> outcome::result<void> { return outcome::success(); };

    virtual ~Codec() = default;

    /**
     * @brief Encode node to byte representation and store children
     * @param node node in the trie
     * @param version the version of trie encoding algorithm
     * @param child_visitor function invoked for every child in a branch node
     * recursively
     * @return encoded representation of a {@param node}
     */
    virtual outcome::result<common::Buffer> encodeNode(
        const TrieNode &node,
        StateVersion version,
        const ChildVisitor &child_visitor = NoopChildVisitor) const = 0;

    /**
     * @brief Decode node from bytes
     * @param encoded_data a buffer containing encoded representation of a node
     * @return a node in the trie
     */
    virtual outcome::result<std::shared_ptr<TrieNode>> decodeNode(
        common::BufferView encoded_data) const = 0;

    /**
     * @brief Get the merkle value of a node
     * @param buf byte representation of the node
     * @return hash of \param buf or \param buf if it is shorter than the hash
     */
    virtual MerkleValue merkleValue(const common::BufferView &buf) const = 0;
    virtual outcome::result<MerkleValue> merkleValue(
        const OpaqueTrieNode &node,
        StateVersion version,
        const ChildVisitor &child_visitor = NoopChildVisitor) const = 0;

    /**
     * @brief Get the hash of a node
     * @param buf byte representation of the node
     * @return hash of \param buf
     */
    virtual common::Hash256 hash256(const common::BufferView &buf) const = 0;

    virtual bool shouldBeHashed(const ValueAndHash &value,
                                StateVersion version) const = 0;
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_TRIE_CODEC_HPP
