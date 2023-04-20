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
    using ChildVisitor = std::function<outcome::result<void>(
        TrieNode const & /* a child node */,
        common::BufferView /* its merkle value */,
        common::Buffer && /* the encoded node */)>;

    static constexpr auto NoopChildVisitor =
        [](TrieNode const &, common::BufferView, common::Buffer &&)
        -> outcome::result<void> { return outcome::success(); };

    virtual ~Codec() = default;

    /**
     * @brief Encode node to byte representation and store children
     * @param node node in the trie
     * @param version the version of trie encoding algorithm, preserve as is if
     * nullopt
     * @param child_visitor function invoked for every child in a branch node
     * recursively
     * @return encoded representation of a {@param node}
     */
    virtual outcome::result<common::Buffer> encodeNode(
        const TrieNode &node,
        std::optional<StateVersion> version,
        const ChildVisitor &child_visitor = NoopChildVisitor) const = 0;

    /**
     * @brief Decode node from bytes
     * @param encoded_data a buffer containing encoded representation of a node
     * @return a node in the trie
     */
    virtual outcome::result<std::shared_ptr<TrieNode>> decodeNode(
        gsl::span<const uint8_t> encoded_data) const = 0;

    /**
     * @brief Get the merkle value of a node
     * @param buf byte representation of the node
     * @return hash of \param buf or \param buf if it is shorter than the hash
     */
    virtual common::Buffer merkleValue(const common::BufferView &buf) const = 0;
    virtual outcome::result<common::Buffer> merkleValue(
        const OpaqueTrieNode &node,
        std::optional<StateVersion> version,
        const ChildVisitor &child_visitor = NoopChildVisitor) const = 0;

    /**
     * @brief is this a hash of value, or value itself
     */
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
