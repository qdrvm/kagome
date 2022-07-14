/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_POLKADOT_NODE
#define KAGOME_STORAGE_TRIE_POLKADOT_NODE

#include <optional>

#include <fmt/format.h>

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

    /**
     * Def. 14 KeyEncode
     * Splits a key to an array of nibbles (a nibble is a half of a byte)
     * TODO(Harrm): Good candidate to rewrite with SIMD
     */
    static KeyNibbles fromByteBuffer(const common::BufferView &key) {
      if (key.empty()) {
        return {};
      }
      if (key.size() == 1 && key[0] == 0) {
        return KeyNibbles{common::Buffer{0, 0}};
      }

      auto l = key.size() * 2;
      KeyNibbles res(common::Buffer(l, 0));
      for (ssize_t i = 0; i < key.size(); i++) {
        res[2 * i] = key[i] >> 4u;
        res[2 * i + 1] = key[i] & 0xfu;
      }

      return res;
    }

    /**
     * Collects an array of nibbles to a key
     * TODO(Harrm): Good candidate to rewrite with SIMD
     */
    Buffer toByteBuffer() const {
      Buffer res;
      if (size() % 2 == 0) {
        res = Buffer(size() / 2, 0);
        for (size_t i = 0; i < size() / 2; i++) {
          res[i] = toByte((*this)[i * 2], (*this)[i * 2 + 1]);
        }
      } else {
        res = Buffer(size() / 2 + 1, 0);
        res[0] = (*this)[0];
        for (size_t i = 1; i < size() / 2 + 1; i++) {
          res[i] = toByte((*this)[i * 2 - 1], (*this)[i * 2]);
        }
      }
      return res;
    }

    static uint8_t toByte(uint8_t high, uint8_t low) {
      return (high << 4u) | (low & 0xfu);
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
      Special,                    // -
      Leaf,                       // 01
      BranchEmptyValue,           // 10
      BranchWithValue,            // 11
      LeafContainingHashes,       // 001
      BranchContainingHashes,     // 0001
      Empty,                      // 0000 0000
      ReservedForCompactEncoding  // 0001 0000
    };

    // just to avoid static_casts every time you need a switch on a node type
    Type getTrieType() const noexcept {
      return static_cast<Type>(getType());
    }

    bool isBranch() const noexcept {
      auto type = getTrieType();
      return type == Type::BranchWithValue or type == Type::BranchEmptyValue
             or type == Type::BranchContainingHashes;
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

  struct BranchContainingHashesNode : public TrieNode {
    static constexpr uint8_t kMaxChildren = 16;

    BranchContainingHashesNode() = default;
    explicit BranchContainingHashesNode(
        KeyNibbles key_nibbles,
        std::optional<common::Buffer> value = std::nullopt)
        : TrieNode{std::move(key_nibbles), std::move(value)} {}

    ~BranchContainingHashesNode() override = default;

    int getType() const override;

    uint16_t childrenBitmap() const;
    uint8_t childrenNum() const;

    // Has 1..16 children.
    // Stores their hashes to search for them in a storage and encode them more
    // easily. @see DummyNode
    std::array<std::shared_ptr<OpaqueTrieNode>, kMaxChildren> children;
  };

  struct LeafContainingHashesNode : public TrieNode {
    LeafContainingHashesNode() = default;
    LeafContainingHashesNode(KeyNibbles key_nibbles,
                             std::optional<common::Buffer> value)
        : TrieNode{std::move(key_nibbles), std::move(value)} {}

    ~LeafContainingHashesNode() override = default;

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

template <>
struct fmt::formatter<kagome::storage::trie::KeyNibbles> {
  template <typename FormatContext>
  auto format(const kagome::storage::trie::KeyNibbles &p, FormatContext &ctx)
      -> decltype(ctx.out()) {
    if (p.size() % 2 != 0) {
      format_to(ctx.out(), "{:x}", p[0]);
    }
    for (size_t i = p.size() % 2; i < p.size() - 1; i += 2) {
      format_to(ctx.out(), "{:02x}", p.toByte(p[i], p[i + 1]));
    }
  }
};

#endif  // KAGOME_STORAGE_TRIE_POLKADOT_NODE
