/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>

#include <fmt/format.h>

#include "common/blob.hpp"
#include "common/buffer.hpp"

namespace kagome::storage::trie {

  using NibblesView = common::BufferView;

  struct KeyNibbles : public common::Buffer {
    using Buffer = common::Buffer;

    using Buffer::Buffer;
    using Buffer::operator<=>;

    KeyNibbles(common::Buffer b) : Buffer{std::move(b)} {}

    /**
     * Def. 14 KeyEncode
     * Splits a key to an array of nibbles (a nibble is a half of a byte)
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
      for (size_t i = 0; i < key.size(); i++) {
        res[2 * i] = key[i] >> 4u;
        res[2 * i + 1] = key[i] & 0xfu;
      }

      return res;
    }

    /**
     * Collects an array of nibbles to a key
     */
    auto toByteBuffer() const {
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

  using MerkleHash = common::Hash256;

  struct MerkleValue final {
   public:
    // the only error is zero size or exceeding size limit, so returns an
    // optional
    static std::optional<MerkleValue> create(common::BufferView merkle_value) {
      auto size = static_cast<size_t>(merkle_value.size());
      if (size == common::Hash256::size()) {
        return MerkleValue{common::Hash256::fromSpan(merkle_value).value(),
                           size};
      } else if (size < common::Hash256::size()) {
        common::Hash256 hash;
        std::copy_n(merkle_value.begin(), size, hash.begin());
        return MerkleValue{hash, size};
      }
      return std::nullopt;
    }

    MerkleValue(const MerkleHash &hash)
        : value{hash}, size{MerkleHash::size()} {}

    bool isHash() const {
      return size == MerkleHash::size();
    }

    std::optional<MerkleHash> asHash() const {
      if (isHash()) {
        return value;
      }
      return std::nullopt;
    }

    common::BufferView asBuffer() const {
      return common::BufferView{value.begin(), value.begin() + size};
    }

    bool empty() const {
      return size == 0;
    }

   private:
    MerkleValue(const common::Hash256 &value, size_t size)
        : value{value}, size{size} {}
    common::Hash256 value;
    size_t size;
  };

  class ValueAndHash {
   public:
    ValueAndHash() = default;
    ValueAndHash(std::optional<common::Buffer> value,
                 std::optional<common::Hash256> hash,
                 bool dirty = true)
        : hash{hash}, value{std::move(value)}, dirty_{dirty} {}

    operator bool() const {
      return is_some();
    }

    bool is_none() const {
      return !is_some();
    }

    bool is_some() const {
      return hash.has_value() || value.has_value();
    }

    bool dirty() const {
      return dirty_;
    }

    std::optional<common::Hash256> hash;
    std::optional<common::Buffer> value;

   private:
    /**
     * Value was inserted or overwritten.
     *
     * Used to convert full value to hash during encoding.
     */
    bool dirty_ = true;
  };

  /**
   * For specification see
   * 5.3 The Trie structure in the Polkadot Host specification
   */

  struct OpaqueTrieNode {
    virtual ~OpaqueTrieNode() = default;
  };

  struct TrieNode : public OpaqueTrieNode {
    TrieNode() = default;
    TrieNode(KeyNibbles key_nibbles, ValueAndHash value)
        : key_nibbles_{std::move(key_nibbles)}, value_{std::move(value)} {}

    enum class Type {
      Special,                 // -
      Leaf,                    // 01
      BranchEmptyValue,        // 10
      BranchWithValue,         // 11
      LeafContainingHashes,    // 001
      BranchContainingHashes,  // 0001
      Empty,                   // 0000 0000
    };

    virtual bool isBranch() const = 0;

    const KeyNibbles &getKeyNibbles() const {
      return key_nibbles_;
    }

    KeyNibbles &getMutKeyNibbles() {
      return key_nibbles_;
    }

    void setKeyNibbles(KeyNibbles &&key_nibbles) {
      key_nibbles_ = std::move(key_nibbles);
    }

    const ValueAndHash &getValue() const {
      return value_;
    }

    ValueAndHash &getMutableValue() {
      return value_;
    }

    void setValue(const ValueAndHash &new_value) {
      value_ = new_value;
    }

    void setValue(ValueAndHash &&new_value) {
      value_ = std::move(new_value);
    }

   private:
    KeyNibbles key_nibbles_;
    ValueAndHash value_;
  };

  struct BranchNode : public TrieNode {
    static constexpr uint8_t kMaxChildren = 16;

    BranchNode() = default;
    explicit BranchNode(KeyNibbles key_nibbles,
                        std::optional<common::Buffer> value = std::nullopt)
        : TrieNode{std::move(key_nibbles), {std::move(value), std::nullopt}} {}

    ~BranchNode() override = default;

    uint8_t getNextChildIdxFrom(uint8_t child_idx) const {
      while (child_idx < kMaxChildren) {
        if (children[child_idx]) {
          return child_idx;
        }
        child_idx++;
      }
      return kMaxChildren;
    }

    virtual bool isBranch() const override {
      return true;
    }

    uint16_t childrenBitmap() const;
    uint8_t childrenNum() const;

    // Has 1..16 children.
    // Stores their hashes to search for them in a storage and encode them more
    // easily. @see DummyNode
    std::array<std::shared_ptr<OpaqueTrieNode>, kMaxChildren> children;
  };

  struct LeafNode : public TrieNode {
    LeafNode() = default;
    LeafNode(KeyNibbles key_nibbles, common::Buffer value)
        : TrieNode{std::move(key_nibbles), {std::move(value), std::nullopt}} {}
    LeafNode(KeyNibbles key_nibbles, ValueAndHash value)
        : TrieNode{std::move(key_nibbles), std::move(value)} {}

    virtual bool isBranch() const override {
      return false;
    }

    ~LeafNode() override = default;
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
    explicit DummyNode(const MerkleValue &key) : db_key{key} {}

    MerkleValue db_key;
  };

}  // namespace kagome::storage::trie

template <>
struct fmt::formatter<kagome::storage::trie::KeyNibbles> {
  template <typename FormatContext>
  auto format(const kagome::storage::trie::KeyNibbles &p,
              FormatContext &ctx) const -> decltype(ctx.out()) {
    if (p.size() % 2 != 0) {
      fmt::format_to(ctx.out(), "{:x}", p[0]);
    }
    for (size_t i = p.size() % 2; i < p.size() - 1; i += 2) {
      fmt::format_to(ctx.out(), "{:02x}", p.toByte(p[i], p[i + 1]));
    }
  }
};
