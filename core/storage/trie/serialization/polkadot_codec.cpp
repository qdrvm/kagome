/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/serialization/polkadot_codec.hpp"

#include "crypto/blake2/blake2b.h"
#include "scale/scale.hpp"
#include "scale/scale_decoder_stream.hpp"
#include "storage/trie/polkadot_trie/polkadot_node.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::storage::trie, PolkadotCodec::Error, e) {
  using E = kagome::storage::trie::PolkadotCodec::Error;
  switch (e) {
    case E::SUCCESS:
      return "success";
    case E::TOO_MANY_NIBBLES:
      return "number of nibbles in key is >= 2**16";
    case E::UNKNOWN_NODE_TYPE:
      return "unknown polkadot node type";
    case E::INPUT_TOO_SMALL:
      return "not enough bytes in the input to decode a node";
    case E::NO_NODE_VALUE:
      return "no value in leaf node";
  }

  return "unknown";
}

namespace kagome::storage::trie {

  inline uint8_t byteFromNibbles(uint8_t high, uint8_t low) {
    // get low4 from nibbles to avoid check: if(low > 0xff) return error
    return (high << 4u) | (low & 0xfu);
  }

  inline common::Buffer ushortToBytes(uint16_t b) {
    common::Buffer out(2, 0);
    out[1] = (b >> 8u) & 0xffu;
    out[0] = b & 0xffu;
    return out;
  }

  common::Buffer PolkadotCodec::nibblesToKey(const common::Buffer &nibbles) {
    Buffer res;
    if (nibbles.size() % 2 == 0) {
      res = Buffer(nibbles.size() / 2, 0);
      for (size_t i = 0; i < nibbles.size(); i += 2) {
        res[i / 2] = byteFromNibbles(nibbles[i], nibbles[i + 1]);
      }
    } else {
      res = Buffer(nibbles.size() / 2 + 1, 0);
      res[0] = nibbles[0];
      for (size_t i = 2; i < nibbles.size(); i += 2) {
        res[i / 2] = byteFromNibbles(nibbles[i - 1], nibbles[i]);
      }
    }
    return res;
  }

  common::Buffer PolkadotCodec::keyToNibbles(const common::Buffer &key) {
    if (key.empty()) {
      return {};
    }
    if (key.size() == 1 && key[0] == 0) {
      return {0, 0};
    }

    auto l = key.size() * 2;
    Buffer res(l, 0);
    for (size_t i = 0; i < key.size(); i++) {
      res[2 * i] = key[i] >> 4u;
      res[2 * i + 1] = key[i] & 0xfu;
    }

    return res;
  }

  common::Buffer PolkadotCodec::merkleValue(const common::Buffer &buf) const {
    // if a buffer size is less than the size of a would-be hash, just return
    // this buffer to save space
    if (buf.size() < common::Hash256::size()) {
      return buf;
    }

    return Buffer{hash256(buf)};
  }

  common::Hash256 PolkadotCodec::hash256(const common::Buffer &buf) const {
    common::Hash256 out;

    blake2b(out.data(),
            common::Hash256::size(),
            nullptr,
            0,
            buf.data(),
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

  outcome::result<common::Buffer> PolkadotCodec::encodeHeader(
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
    // the first two bits are a node type
    head <<= 6u;  // head_{6..7} * 64

    // set bits 0..5, partial key length
    if (node.key_nibbles.size() < 63u) {
      head |= uint8_t(node.key_nibbles.size());
      return Buffer{head};  // header contains 1 byte
    }
    // if partial key length is greater than 62, then the rest of the length is
    // stored in consequent bytes
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
    OUTCOME_TRY(encoding, encodeHeader(node));

    // key
    encoding += nibblesToKey(node.key_nibbles);

    // children bitmap
    encoding += ushortToBytes(node.childrenBitmap());

    if (node.getTrieType() == PolkadotNode::Type::BranchWithValue) {
      // scale encoded value
      OUTCOME_TRY(encNodeValue, scale::encode(node.value.get()));
      encoding += Buffer(std::move(encNodeValue));
    }

    // encode each child
    for (auto &child : node.children) {
      if (child) {
        if (child->isDummy()) {
          auto merkle_value =
              std::dynamic_pointer_cast<DummyNode>(child)->db_key;
          OUTCOME_TRY(scale_enc, scale::encode(std::move(merkle_value)));
          encoding.put(scale_enc);
        } else {
          OUTCOME_TRY(enc, encodeNode(*child));
          OUTCOME_TRY(scale_enc, scale::encode(merkleValue(enc)));
          encoding.put(scale_enc);
        }
      }
    }

    return outcome::success(std::move(encoding));
  }

  outcome::result<common::Buffer> PolkadotCodec::encodeLeaf(
      const LeafNode &node) const {
    OUTCOME_TRY(encoding, encodeHeader(node));

    // key
    encoding += nibblesToKey(node.key_nibbles);

    if (!node.value) return Error::NO_NODE_VALUE;
    // scale encoded value
    OUTCOME_TRY(encNodeValue, scale::encode(node.value.get()));
    encoding += Buffer(std::move(encNodeValue));

    return outcome::success(std::move(encoding));
  }

  outcome::result<std::shared_ptr<Node>> PolkadotCodec::decodeNode(
      const common::Buffer &encoded_data) const {
    BufferStream stream{encoded_data};
    // decode the header with the node type and the partial key length
    OUTCOME_TRY(header, decodeHeader(stream));
    auto [type, pk_length] = header;
    // decode the partial key
    OUTCOME_TRY(partial_key, decodePartialKey(pk_length, stream));
    // decode the node subvalue (see Definition 28 of the polkadot
    // specification)
    switch (type) {
      case PolkadotNode::Type::Leaf: {
        OUTCOME_TRY(value, scale::decode<Buffer>(stream.leftBytes()));
        return std::make_shared<LeafNode>(partial_key, value);
      }
      case PolkadotNode::Type::BranchEmptyValue:
      case PolkadotNode::Type::BranchWithValue: {
        return decodeBranch(type, partial_key, stream);
      }
      default:
        return Error::UNKNOWN_NODE_TYPE;
    }
  }

  outcome::result<std::pair<PolkadotNode::Type, size_t>>
  PolkadotCodec::decodeHeader(BufferStream &stream) const {
    PolkadotNode::Type type;
    size_t pk_length = 0;
    if (not stream.hasMore(1)) {
      return Error::INPUT_TOO_SMALL;
    }
    auto first = stream.next();
    // decode type, which is in the first two bits
    type = static_cast<PolkadotNode::Type>((first & 0xC0u) >> 6u);
    first &= 0x3Fu;
    // decode partial key length, which is stored in the last 6 bits and, if its
    // greater than 62, in the following bytes
    pk_length += first;

    if (pk_length == 63) {
      uint8_t read_length = 0;
      do {
        if (not stream.hasMore(1)) {
          return Error::INPUT_TOO_SMALL;
        }
        read_length = stream.next();
        pk_length += read_length;
      } while (read_length == 0xFF);
    }
    return std::make_pair(type, pk_length);
  }

  outcome::result<common::Buffer> PolkadotCodec::decodePartialKey(
      size_t nibbles_num, BufferStream &stream) const {
    // length in bytes is length in nibbles over two round up
    auto byte_length = nibbles_num / 2 + nibbles_num % 2;
    Buffer partial_key;
    partial_key.reserve(byte_length);
    while (byte_length-- != 0) {
      if (not stream.hasMore(1)) {
        return Error::INPUT_TOO_SMALL;
      }
      partial_key.putUint8(stream.next());
    }
    // array of nibbles is much more convenient than array of bytes, though it
    // wastes some memory
    partial_key = keyToNibbles(partial_key);
    if (nibbles_num % 2 == 1) {
      partial_key = partial_key.subbuffer(1);
    }
    return partial_key;
  }

  outcome::result<std::shared_ptr<Node>> PolkadotCodec::decodeBranch(
      PolkadotNode::Type type,
      const Buffer &partial_key,
      BufferStream &stream) const {
    constexpr uint8_t kChildrenBitmapSize = 2;

    if (not stream.hasMore(kChildrenBitmapSize)) {
      return Error::INPUT_TOO_SMALL;
    }
    auto node = std::make_shared<BranchNode>(partial_key);

    uint16_t children_bitmap = stream.next();
    children_bitmap += stream.next() << 8u;

    scale::ScaleDecoderStream ss(stream.leftBytes());

    // decode the branch value if needed
    common::Buffer value;
    if (type == PolkadotNode::Type::BranchWithValue) {
      try {
        ss >> value;
      } catch (std::system_error &e) {
        return outcome::failure(e.code());
      }
      node->value = value;
    }

    uint8_t i = 0;
    while (children_bitmap != 0) {
      // if there is a child
      if ((children_bitmap & (1u << i)) != 0) {
        // unset bit for this child
        children_bitmap &= ~(1u << i);
        // read the hash of the child and make a dummy node from it for this
        // child in the processed branch
        common::Buffer child_hash;
        try {
          ss >> child_hash;
        } catch (std::system_error &e) {
          return outcome::failure(e.code());
        }
        node->children.at(i) = std::make_shared<DummyNode>(child_hash);
      }
      i++;
    }
    return node;
  }

}  // namespace kagome::storage::trie
