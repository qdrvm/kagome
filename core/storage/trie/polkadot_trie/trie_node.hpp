/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_POLKADOT_NODE
#define KAGOME_STORAGE_TRIE_POLKADOT_NODE

#include <optional>

#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "storage/trie/node.hpp"

namespace kagome::storage::trie {

  using NibblesView = common::BufferView;

  struct KeyNibbles : public common::Buffer {
    KeyNibbles() = default;

    explicit KeyNibbles(common::Buffer b) : Buffer{std::move(b)} {}
    KeyNibbles(std::initializer_list<uint8_t> b) : Buffer{b} {}
    KeyNibbles(NibblesView b) : Buffer{b} {}

    KeyNibbles &operator=(common::Buffer b) {
      Buffer::operator=(std::move(b));
      return *this;
    }

    NibblesView subspan(size_t offset = 0, size_t length = -1) const {
      return NibblesView{*this}.subspan(offset, length);
    }
  };

  /**
   * For specification see
   * 5.3 The Trie structure in the Polkadot Host specification
   */

  struct OpaqueTrieNode : public Node {};

  struct TrieNode : public OpaqueTrieNode {
    TrieNode() = default;
    TrieNode(KeyNibbles key_nibbles, std::optional<common::Buffer> value)
        : key_nibbles{std::move(key_nibbles)}, value{std::move(value)} {}

    ~TrieNode() override = default;

    enum class Type {
      Special = 0b00,
      Leaf = 0b01,
      BranchEmptyValue = 0b10,
      BranchWithValue = 0b11
    };

    // just to avoid static_casts every time you need a switch on a node type
    Type getTrieType() const noexcept {
      return static_cast<Type>(getType());
    }

    bool isBranch() const noexcept {
      auto type = getTrieType();
      return type == Type::BranchWithValue or type == Type::BranchEmptyValue;
    }

    KeyNibbles key_nibbles;
    std::optional<common::Buffer> value;
  };

  struct BranchNode : public TrieNode {
    static constexpr uint8_t kMaxChildren = 16;

    BranchNode() = default;
    explicit BranchNode(KeyNibbles key_nibbles,
                        std::optional<common::Buffer> value = std::nullopt)
        : TrieNode{std::move(key_nibbles), std::move(value)} {}

    ~BranchNode() override = default;

    int getType() const override;

    uint16_t childrenBitmap() const;
    uint8_t childrenNum() const;

    // Has 1..16 children.
    // Stores their hashes to search for them in a storage and encode them more
    // easily. @see DummyNode
    std::array<std::shared_ptr<OpaqueTrieNode>, kMaxChildren> children;
  };

  struct LeafNode : public TrieNode {
    LeafNode() = default;
    LeafNode(KeyNibbles key_nibbles, std::optional<common::Buffer> value)
        : TrieNode{std::move(key_nibbles), std::move(value)} {}

    ~LeafNode() override = default;

    int getType() const override;
  };

  /**
   * Used in branch nodes to indicate that there is a node, but this node is not
   * interesting at the moment and need not be retrieved from the storage.
   */
  struct DummyNode : public OpaqueTrieNode {
    /**
     * Constructs a dummy node
     * @param key a storage key, which is a hash of an encoded node according to
     * PolkaDot specification
     */
    explicit DummyNode(common::Buffer key) : db_key{std::move(key)} {}

    int getType() const override {
      // Special only because a node has to have a type. Actually this is not
      // the real node and the type of the underlying node is inaccessible
      // before reading from the storage
      return static_cast<int>(TrieNode::Type::Special);
    }

    common::Buffer db_key;
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_STORAGE_TRIE_POLKADOT_NODE
