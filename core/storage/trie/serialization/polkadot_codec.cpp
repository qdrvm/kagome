/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/trie/serialization/polkadot_codec.hpp"

#include "crypto/blake2/blake2b.h"
#include "log/logger.hpp"
#include "scale/kagome_scale.hpp"
#include "storage/trie/polkadot_trie/trie_node.hpp"

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
  constexpr size_t kMaxInlineValueSizeVersion1 = 33;

  inline common::Buffer ushortToBytes(uint16_t b) {
    common::Buffer out(2, 0);
    out[1] = (b >> 8u) & 0xffu;
    out[0] = b & 0xffu;
    return out;
  }

  bool PolkadotCodec::shouldBeHashed(const ValueAndHash &value,
                                     StateVersion version) const {
    if (value.hash || !value.value) {
      return false;
    }
    switch (version) {
      case StateVersion::V0: {
        return false;
      }
      case StateVersion::V1: {
        if (!value.dirty()) {
          return false;
        }
        return value.value->size() >= kMaxInlineValueSizeVersion1;
      }
    }
    BOOST_UNREACHABLE_RETURN();
  }

  inline TrieNode::Type getType(const TrieNode &node) {
    if (node.isBranch()) {
      if (node.getValue().hash) {
        return TrieNode::Type::BranchContainingHashes;
      }
      if (node.getValue().value) {
        return TrieNode::Type::BranchWithValue;
      }
      return TrieNode::Type::BranchEmptyValue;
    }
    if (node.getValue().hash) {
      return TrieNode::Type::LeafContainingHashes;
    }
    if (node.getValue().value) {
      return TrieNode::Type::Leaf;
    }
    return TrieNode::Type::Empty;
  }

  MerkleValue PolkadotCodec::merkleValue(const common::BufferView &buf) const {
    // if a buffer size is less than the size of a would-be hash, just return
    // this buffer to save space
    if (static_cast<size_t>(buf.size()) < common::Hash256::size()) {
      return *MerkleValue::create(buf);
    }

    return MerkleValue{hash256(buf)};
  }

  outcome::result<MerkleValue> PolkadotCodec::merkleValue(
      const OpaqueTrieNode &node,
      StateVersion version,
      TraversePolicy policy,
      const ChildVisitor &child_visitor) const {
    if (node.isDummy()) {
      ++stats_.node_cache_hits;
      return node.asDummy().db_key;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    auto &trie_node = static_cast<const TrieNode &>(node);

    if (auto cached_merkle_value = trie_node.getMerkleCache();
        cached_merkle_value.has_value()) {
      ++stats_.node_cache_hits;
      return *cached_merkle_value;
    }
    OUTCOME_TRY(enc, encodeNode(trie_node, version, policy, child_visitor));

    auto merkle_value = merkleValue(enc);
    if (merkle_value.isHash()) {
      trie_node.setMerkleCache(*merkle_value.asHash());
    }

    return merkle_value;
  }

  common::Hash256 PolkadotCodec::hash256(const common::BufferView &buf) const {
    return hash_func_(buf);
  }

  outcome::result<common::Buffer> PolkadotCodec::encodeNode(
      const TrieNode &node,
      StateVersion version,
      TraversePolicy policy,
      const ChildVisitor &child_visitor) const {
    outcome::result<common::Buffer> res{{}};
    if (node.isBranch()) {
      res = encodeBranch(node.asBranch(), version, policy, child_visitor);
    } else {
      res = encodeLeaf(node.asLeaf(), version, child_visitor);
    }
    if (res) {
      ++stats_.encoded_nodes;
      stats_.total_encoded_nodes_size += res.value().size();
    }
    return res;
  }

  outcome::result<common::Buffer> PolkadotCodec::encodeHeader(
      const TrieNode &node, StateVersion version) const {
    if (node.getKeyNibbles().size() > 0xffffu) {
      return Error::TOO_MANY_NIBBLES;
    }

    auto type = getType(node);
    if (shouldBeHashed(node.getValue(), version)) {
      if (node.isBranch()) {
        type = TrieNode::Type::BranchContainingHashes;
      } else {
        type = TrieNode::Type::LeafContainingHashes;
      }
    }

    uint8_t head;  // NOLINT(cppcoreguidelines-init-variables)

    // max partial key length
    uint8_t partial_length_mask;  // NOLINT(cppcoreguidelines-init-variables)

    // set bits of type
    switch (type) {
      case TrieNode::Type::Leaf:
        head = 0b01'000000;
        partial_length_mask = 0b00'111111;  // 63
        break;
      case TrieNode::Type::BranchEmptyValue:
        head = 0b10'000000;
        partial_length_mask = 0b00'111111;  // 63
        break;
      case TrieNode::Type::BranchWithValue:
        head = 0b11'000000;
        partial_length_mask = 0b00'111111;  // 63
        break;
      case TrieNode::Type::LeafContainingHashes:
        head = 0b001'00000;
        partial_length_mask = 0b000'11111;  // 31
        break;
      case TrieNode::Type::BranchContainingHashes:
        head = 0b0001'0000;
        partial_length_mask = 0b0000'1111;  // 15
        break;
      case TrieNode::Type::Empty:
        head = 0b00000000;
        partial_length_mask = 0;
        break;
      default:
        return Error::UNKNOWN_NODE_TYPE;
    }

    // set bits of partial key length
    if (node.getKeyNibbles().size() < partial_length_mask) {
      head |= uint8_t(node.getKeyNibbles().size());
      return Buffer{head};  // header contains 1 byte
    }
    // if partial key length is greater than max partial key length, then the
    // rest of the length is stored in consequent bytes
    head += partial_length_mask;

    size_t l = node.getKeyNibbles().size() - partial_length_mask;
    Buffer out(1u +             // 1 byte head
                   l / 0xffu +  // number of 255 in l
                   1u,          // for last byte
               0xffu            // fill array with 255
    );

    // first byte is head
    out[0] = head;
    // last byte after while(l>255) l-=255;
    out[out.size() - 1] = l % 0xffu;

    return out;
  }

  outcome::result<void> PolkadotCodec::encodeValue(
      common::Buffer &out,
      const TrieNode &node,
      StateVersion version,
      const ChildVisitor &child_visitor) const {
    auto hash = node.getValue().hash;
    if (shouldBeHashed(node.getValue(), version)) {
      hash = hash256(*node.getValue().value);
      if (child_visitor) {
        OUTCOME_TRY(
            child_visitor(ValueData{node, *hash, *node.getValue().value}));
      }
    }
    if (hash) {
      out += *hash;
    } else if (node.getValue().value) {
      OUTCOME_TRY(value, scale::encode(*node.getValue().value));
      out += std::move(value);
    }
    ++stats_.encoded_values;
    stats_.total_encoded_values_size += out.size();
    return outcome::success();
  }

  outcome::result<common::Buffer> PolkadotCodec::encodeBranch(
      const BranchNode &node,
      StateVersion version,
      TraversePolicy policy,
      const ChildVisitor &child_visitor) const {
    // node header
    OUTCOME_TRY(encoding, encodeHeader(node, version));

    // key
    encoding += node.getKeyNibbles().toByteBuffer();

    // children bitmap
    encoding += ushortToBytes(node.childrenBitmap());

    OUTCOME_TRY(encodeValue(encoding, node, version, child_visitor));

    // encode each child
    for (auto &child : node.getChildren()) {
      if (child) {
        if (child->isDummy()) {
          auto merkle_value = child->asDummy().db_key;
          OUTCOME_TRY(scale_enc, scale::encode(merkle_value.asBuffer()));
          encoding.put(scale_enc);
        } else {
          // because a node is either a dummy or a trienode
          auto &child_node = dynamic_cast<TrieNode &>(*child);

          // use optional because MerkleValue doesn't have a default constructor
          std::optional<MerkleValue> merkle;

          // if TraversePolicy is UncachedOnly, we don't need to call
          // child_visitor on nodes with cached merkle value, therefore we don't
          // need to encode them (child_visitor requires a node's encoding) and
          // can just take the cached merkle value
          if (policy == TraversePolicy::UncachedOnly
              && child_node.getMerkleCache().has_value()) {
            merkle = child_node.getMerkleCache().value();
          } else {
            OUTCOME_TRY(enc,
                        encodeNode(child_node, version, policy, child_visitor));
            merkle = merkleValue(enc);
            if (merkle->isHash()) {
              if (child_visitor) {
                OUTCOME_TRY(child_visitor(
                    ChildData{child_node, *merkle, std::move(enc)}));
              }
              if (!child_node.getMerkleCache().has_value()) {
                child_node.setMerkleCache(merkle->asHash());
              }
            }
          }

          OUTCOME_TRY(scale_enc, scale::encode(merkle->asBuffer()));
          encoding.put(scale_enc);
        }
      }
    }

    return outcome::success(std::move(encoding));
  }

  outcome::result<common::Buffer> PolkadotCodec::encodeLeaf(
      const LeafNode &node,
      StateVersion version,
      const ChildVisitor &child_visitor) const {
    OUTCOME_TRY(encoding, encodeHeader(node, version));

    // key
    encoding += node.getKeyNibbles().toByteBuffer();

    if (!node.getValue()) {
      return Error::NO_NODE_VALUE;
    }

    OUTCOME_TRY(encodeValue(encoding, node, version, child_visitor));

    return outcome::success(std::move(encoding));
  }

  outcome::result<std::shared_ptr<TrieNode>> PolkadotCodec::decodeNode(
      common::BufferView encoded_data) const {
    BufferStream stream{encoded_data};
    stats_.total_decoded_nodes_size += encoded_data.size();
    ++stats_.decoded_nodes;
    // decode the header with the node type and the partial key length
    OUTCOME_TRY(header, decodeHeader(stream));
    auto [type, pk_length] = header;
    // decode the partial key
    OUTCOME_TRY(partial_key, decodePartialKey(pk_length, stream));
    // decode the node sub-value (see Definition 28 of the polkadot
    // specification)
    switch (type) {
      case TrieNode::Type::Leaf: {
        OUTCOME_TRY(value, scale::decode<Buffer>(stream.leftBytes()));
        return std::make_shared<LeafNode>(
            partial_key, ValueAndHash{std::move(value), std::nullopt, false});
      }

      case TrieNode::Type::BranchEmptyValue:
      case TrieNode::Type::BranchWithValue:
        return decodeBranch(type, partial_key, stream);

      case TrieNode::Type::LeafContainingHashes: {
        OUTCOME_TRY(hash, scale::decode<common::Hash256>(stream.leftBytes()));
        return std::make_shared<LeafNode>(
            partial_key, ValueAndHash{std::nullopt, hash, false});
      }

      case TrieNode::Type::BranchContainingHashes:
        return decodeBranch(type, partial_key, stream);

      case TrieNode::Type::Empty:
        return Error::UNKNOWN_NODE_TYPE;

      default:
        return Error::UNKNOWN_NODE_TYPE;
    }
  }

  outcome::result<std::pair<TrieNode::Type, size_t>>
  PolkadotCodec::decodeHeader(BufferStream &stream) const {
    TrieNode::Type type;  // NOLINT(cppcoreguidelines-init-variables)
    if (not stream.hasMore(1)) {
      return Error::INPUT_TOO_SMALL;
    }
    auto first = stream.next();

    uint8_t partial_key_length_mask = 0;
    if (first & 0b1100'0000) {
      if ((first >> 6 & 0b11) == 0b01) {
        type = TrieNode::Type::Leaf;
      } else if ((first >> 6 & 0b11) == 0b10) {
        type = TrieNode::Type::BranchEmptyValue;
      } else if ((first >> 6 & 0b11) == 0b11) {
        type = TrieNode::Type::BranchWithValue;
      } else {
        BOOST_UNREACHABLE_RETURN();
      }
      partial_key_length_mask = 0b00'111111;
    } else if (first & 0b0010'0000) {
      type = TrieNode::Type::LeafContainingHashes;
      partial_key_length_mask = 0b000'11111;
    } else if (first & 0b0001'0000) {
      type = TrieNode::Type::BranchContainingHashes;
      partial_key_length_mask = 0b0000'1111;
    } else if (first == 0) {
      type = TrieNode::Type::Empty;
    } else {
      BOOST_UNREACHABLE_RETURN();
    }

    // decode partial key length, which is stored in the last bits and,
    // if it's greater than max partial key length, in the following bytes
    size_t pk_length = first & partial_key_length_mask;

    if (pk_length == partial_key_length_mask) {
      uint8_t read_length{};
      do {  // NOLINT(cppcoreguidelines-avoid-do-while)
        if (not stream.hasMore(1)) {
          return Error::INPUT_TOO_SMALL;
        }
        read_length = stream.next();
        pk_length += read_length;
      } while (read_length == 0xFF);
    }
    return std::make_pair(type, pk_length);
  }

  outcome::result<KeyNibbles> PolkadotCodec::decodePartialKey(
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
    auto partial_key_nibbles = KeyNibbles::fromByteBuffer(partial_key);
    if (nibbles_num % 2 == 1) {
      partial_key_nibbles = partial_key_nibbles.subbuffer(1);
    }
    return partial_key_nibbles;
  }

  outcome::result<std::shared_ptr<TrieNode>> PolkadotCodec::decodeBranch(
      TrieNode::Type type,
      const KeyNibbles &partial_key,
      BufferStream &stream) const {
    constexpr uint8_t kChildrenBitmapSize = 2;

    if (not stream.hasMore(kChildrenBitmapSize)) {
      return Error::INPUT_TOO_SMALL;
    }
    auto node = std::make_shared<BranchNode>(partial_key);

    uint16_t children_bitmap = stream.next();
    children_bitmap += stream.next() << 8u;

    scale::DecoderFromSpan decoder(stream.leftBytes());

    // decode the branch value if needed
    common::Buffer value;
    if (type == TrieNode::Type::BranchWithValue) {
      try {
        decode(value, decoder);
      } catch (std::system_error &e) {
        return outcome::failure(e.code());
      }
      node->setValue({std::move(value), std::nullopt, false});
    } else if (type == TrieNode::Type::BranchContainingHashes) {
      common::Hash256 hash;
      try {
        decode(hash, decoder);
      } catch (std::system_error &e) {
        return outcome::failure(e.code());
      }
      node->setValue({std::nullopt, hash, false});
    } else if (type != TrieNode::Type::BranchEmptyValue) {
      return Error::UNKNOWN_NODE_TYPE;
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
          decode(child_hash, decoder);
        } catch (std::system_error &e) {
          return outcome::failure(e.code());
        }
        // SAFETY: database cannot contain invalid merkle values
        node->setChild(i,
                       std::make_shared<DummyNode>(
                           MerkleValue::create(child_hash).value()));
      }
      i++;
    }
    return node;
  }

}  // namespace kagome::storage::trie
