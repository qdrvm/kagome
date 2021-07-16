/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_POLKADOT_NODE
#define KAGOME_STORAGE_TRIE_POLKADOT_NODE

#include <boost/optional.hpp>

#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "storage/trie/node.hpp"

namespace kagome::storage::trie {

  struct KeyNibbles : public common::Buffer {
    KeyNibbles() = default;

    explicit KeyNibbles(common::Buffer b) : Buffer{std::move(b)} {}
    KeyNibbles(std::initializer_list<uint8_t> b) : Buffer{b} {}

    KeyNibbles &operator=(common::Buffer b) {
      Buffer::operator=(std::move(b));
      return *this;
    }

    KeyNibbles subspan(size_t offset = 0, size_t length = -1) const {
      return KeyNibbles{Buffer::subbuffer(offset, length)};
    }
  };

  /**
   * For specification see
   * https://github.com/w3f/polkadot-re-spec/blob/master/polkadot_re_spec.pdf
   * 5.3 The Trie structure
   */

  struct PolkadotNode : public Node {
    PolkadotNode() = default;
    PolkadotNode(KeyNibbles key_nibbles, boost::optional<common::Buffer> value)
        : key_nibbles{std::move(key_nibbles)}, value{std::move(value)} {}

    ~PolkadotNode() override = default;

    enum class Type {
      Special = 0b00,
      Leaf = 0b01,
      BranchEmptyValue = 0b10,
      BranchWithValue = 0b11
    };

    // dummy nodes are used to avoid unnecessary reads from the storage
    virtual bool isDummy() const = 0;

    // just to avoid static_casts every time you need a switch on a node type
    Type getTrieType() const noexcept {
      return static_cast<Type>(getType());
    }

    bool isBranch() const noexcept {
      auto type = getTrieType();
      return type == Type::BranchWithValue or type == Type::BranchEmptyValue;
    }

    KeyNibbles key_nibbles;
    boost::optional<common::Buffer> value;
  };

  struct BranchNode : public PolkadotNode {
    static constexpr int kMaxChildren = 16;

    BranchNode() = default;
    explicit BranchNode(KeyNibbles key_nibbles,
                        boost::optional<common::Buffer> value = boost::none)
        : PolkadotNode{std::move(key_nibbles), std::move(value)} {}

    ~BranchNode() override = default;

    bool isDummy() const override {
      return false;
    }
    int getType() const override;

    uint16_t childrenBitmap() const;
    uint8_t childrenNum() const;

    // Has 1..16 children.
    // Stores their hashes to search for them in a storage and encode them more
    // easily. @see DummyNode
    std::array<std::shared_ptr<PolkadotNode>, kMaxChildren> children;
  };

  struct LeafNode : public PolkadotNode {
    LeafNode() = default;
    LeafNode(KeyNibbles key_nibbles, boost::optional<common::Buffer> value)
        : PolkadotNode{std::move(key_nibbles), std::move(value)} {}

    ~LeafNode() override = default;

    bool isDummy() const override {
      return false;
    }
    int getType() const override;
  };

  /**
   * Used in branch nodes to indicate that there is a node, but this node is not
   * interesting at the moment and need not be retrieved from the storage.
   */
  struct DummyNode : public PolkadotNode {
    /**
     * Constructs a dummy node
     * @param key a storage key, which is a hash of an encoded node according to
     * PolkaDot specification
     */
    explicit DummyNode(common::Buffer key) : db_key{std::move(key)} {}

    bool isDummy() const override {
      return true;
    }

    int getType() const override {
      // Special only because a node has to have a type. Actually this is not
      // the real node and the type of the underlying node is inaccessible
      // before reading from the storage
      return static_cast<int>(Type::Special);
    }

    common::Buffer db_key;
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_STORAGE_TRIE_POLKADOT_NODE
