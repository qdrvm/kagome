/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/merkle/polkadot_trie_db/polkadot_codec.hpp"

#include "crypto/blake2/blake2s.h"
#include "storage/merkle/polkadot_trie_db/polkadot_node.hpp"
#include "scale/scale.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::storage::merkle, PolkadotCodec::Error, e) {
  using E = kagome::storage::merkle::PolkadotCodec::Error;
  switch (e) {
    case E::SUCCESS:
      return "success";
    case E::TOO_MANY_NIBBLES:
      return "number of nibbles in key is >= 2**16";
    case E::UNKNOWN_NODE_TYPE:
      return "unknown polkadot node type";
    case E::INPUT_TOO_SMALL:
      return "not enough bytes in the input to decode a node";
  }

  return "unknown";
}

namespace kagome::storage::merkle {

  inline uint8_t lowNibble(uint8_t byte) {
    return byte & 0xFu;
  }

  inline uint8_t highNibble(uint8_t byte) {
    return (byte >> 4u) & 0xFu;
  }

  inline uint8_t collectByte(uint8_t low, uint8_t high) {
    // get low4 from nibbles to avoid check: if(low > 0xff) return error
    return (lowNibble(high) << 4u) | lowNibble(low);
  }

  inline common::Buffer ushortToBytes(uint16_t b) {
    common::Buffer out(2, 0);
    out[0] = b & 0xffu;
    out[1] = (b >> 8u) & 0xffu;
    return out;
  }

  common::Buffer PolkadotCodec::nibblesToKey(
      const common::Buffer &nibbles) const {
    const auto nibbles_size = nibbles.size();
    if (nibbles_size == 0) {
      return {};
    }

    if (nibbles_size == 1 && nibbles[0] == 0) {
      return {0};
    }

    // if nibbles_size is odd, then allocate one more item
    const size_t size = (nibbles_size + 1) / 2;
    common::Buffer out(size, 0);

    size_t iterations = size;

    // if number of nibbles is odd, then iterate even number of times
    bool nimbles_size_odd = nibbles_size % 2 != 0;
    if (nimbles_size_odd) {
      --iterations;
    }

    for (size_t i = 0; i < iterations; ++i) {
      out[i] = collectByte(nibbles[2 * i], nibbles[2 * i + 1]);
    }

    // if number of nibbles is odd, then implicitly add 0 as very last nibble
    if (nimbles_size_odd) {
      out[iterations] = collectByte(nibbles[2 * iterations], 0);
    }

    return out;
  }

  common::Buffer PolkadotCodec::keyToNibbles(const common::Buffer &key) {
    auto nibbles = key.size() * 2;

    // if last nibble in `key` is 0
    bool last_nibble_0 =
        !key.empty() && (highNibble(key[key.size() - 1]) == 0);
    if (last_nibble_0) {
      --nibbles;
    }

    common::Buffer out(nibbles, 0);
    for (size_t i = 0u, size = nibbles / 2; i < size; i++) {
      out[2 * i] = lowNibble(key[i]);
      out[2 * i + 1] = highNibble(key[i]);
    }

    if (last_nibble_0) {
      out[nibbles - 1] = lowNibble(key[key.size() - 1]);
    }

    return out;
  }

  common::Hash256 PolkadotCodec::hash256(const common::Buffer &buf) const {
    common::Hash256 out;

    if (buf.size() < common::Hash256::size()) {
      std::copy(buf.begin(), buf.end(), out.begin());
      return out;
    }

    blake2s(out.data(), common::Hash256::size(), nullptr, 0, buf.data(),
            buf.size());
    return out;
  }

  outcome::result<common::Buffer> PolkadotCodec::encodeNode(
      const Node &node) const {
    switch (static_cast<PolkadotNode::Type>(node.getType())) {
      case PolkadotNode::Type::BranchEmptyValue:
      case PolkadotNode::Type::BranchWithValue:
        return encodeBranch(dynamic_cast<const BranchNode &>(node));
      case PolkadotNode::Type::Leaf:
        return encodeLeaf(dynamic_cast<const LeafNode &>(node));

      case PolkadotNode::Type::Special:
        // special node is not handled right now
      default:
        return std::errc::invalid_argument;
    }
  }

  outcome::result<std::shared_ptr<Node>> PolkadotCodec::decodeNode(
      common::ByteStream &stream) const {
    PolkadotNode::Type type;
    size_t pk_length = 0;
    if (not stream.hasMore(1)) {
      return Error::INPUT_TOO_SMALL;
    }
    auto first = stream.nextByte().value();
    type = static_cast<PolkadotNode::Type>((first & 0xC0u) >> 6u);
    first &= 0x3Fu;
    pk_length += first;

    if (pk_length == 63) {
      uint8_t read_length = 0;
      do {
        if (not stream.hasMore(1))
          return Error::INPUT_TOO_SMALL;
        read_length = stream.nextByte().value();
        pk_length += read_length;
      } while (read_length == 0xFF);
    }
    auto pk_length_in_bytes = pk_length / 2 + pk_length % 2;
    Buffer partial_key;
    partial_key.reserve(pk_length_in_bytes);
    while (pk_length_in_bytes--) {
      if (not stream.hasMore(1))
        return Error::INPUT_TOO_SMALL;
      partial_key.putUint8(stream.nextByte().value());
    }
    partial_key = keyToNibbles(partial_key);
    switch (type) {
      case PolkadotNode::Type::Leaf: {
        OUTCOME_TRY(value, scale_->decode(stream));
        return std::make_shared<LeafNode>(partial_key, value);
      }
      case PolkadotNode::Type::BranchEmptyValue:
      case PolkadotNode::Type::BranchWithValue: {
        if (not stream.hasMore(2))
          return Error::INPUT_TOO_SMALL;
        auto node = std::make_shared<BranchNode>(partial_key);

        uint16_t children_bitmap = stream.nextByte().value();
        children_bitmap <<= 8u;
        children_bitmap += stream.nextByte().value();

        uint8_t i = 0;
        while (children_bitmap) {
          if (children_bitmap & (1u << i)) {
            children_bitmap &= ~(1u << i);
            if (not stream.hasMore(common::Hash256::size()))
              return Error::INPUT_TOO_SMALL;
            common::Buffer child_hash(32, 0);
            for (auto &b : child_hash) {
              b = stream.nextByte().value();
            }
            node->children[i] = std::make_shared<DummyNode>(child_hash);
          }
          i++;
        }
        if (type == PolkadotNode::Type::BranchWithValue) {
          OUTCOME_TRY(value, scale_->decode(stream));
          node->value = value;
        }
        return node;
      }
      default:
        return Error::UNKNOWN_NODE_TYPE;
    }
  }

  outcome::result<common::Buffer> PolkadotCodec::getHeader(
      const PolkadotNode &node) const {
    if (node.key_nibbles.size() > 0xffffu) {
      return Error::TOO_MANY_NIBBLES;
    }

    uint8_t head = 0;
    // set bits 6..7
    switch (static_cast<PolkadotNode::Type>(node.getType())) {
      case PolkadotNode::Type::BranchEmptyValue:
      case PolkadotNode::Type::BranchWithValue:
      case PolkadotNode::Type::Leaf:
        head = node.getType();
        break;
      default:
        return Error::UNKNOWN_NODE_TYPE;
    }
    head <<= 6u;  // head_{6..7} * 64

    // set bits 0..5
    if (node.key_nibbles.size() < 63u) {
      head |= uint8_t(node.key_nibbles.size());
      return Buffer{head};  // header contains 1 byte
    }

    head += 63u;

    size_t l = node.key_nibbles.size() - 63u;
    Buffer out(1u +             /// 1 byte head
                   l / 0xffu +  /// number of 255 in l
                   1u,          /// for last byte
               0xffu            /// fill array with 255
    );

    // first byte is head
    out[0] = head;
    // last byte after while(l>255) l-=255;
    out[out.size() - 1] = l % 0xffu;

    return out;
  }

  outcome::result<common::Buffer> PolkadotCodec::encodeBranch(
      const BranchNode &node) const {
    // node header
    OUTCOME_TRY(encoding, getHeader(node));

    // key
    encoding += nibblesToKey(node.key_nibbles);

    // children bitmap
    encoding += ushortToBytes(node.childrenBitmap());

    // encode each child
    for (auto &child : node.children) {
      if (child) {
        if(child->isDummy()) {
          auto hash = std::dynamic_pointer_cast<DummyNode>(child)->db_key;
          encoding.putBuffer(hash);
        } else {
          OUTCOME_TRY(enc, encodeNode(*child));
          encoding.put(hash256(enc));
        }
      }
    }

    // scale encoded value
    OUTCOME_TRY(encNodeValue, scale::encode(node.value));
    encoding += Buffer(std::move(encNodeValue));

    return outcome::success(std::move(encoding));
  }

  outcome::result<common::Buffer> PolkadotCodec::encodeLeaf(
      const LeafNode &node) const {
    OUTCOME_TRY(encoding, getHeader(node));

    // key
    encoding += nibblesToKey(node.key_nibbles);

    // scale encoded value
    OUTCOME_TRY(encNodeValue, scale::encode(node.value));
    encoding += Buffer(std::move(encNodeValue));

    return outcome::success(std::move(encoding));
  }

}  // namespace kagome::storage::merkle
