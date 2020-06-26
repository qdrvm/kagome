/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <memory>

#include <gtest/gtest.h>
#include "storage/trie/polkadot_trie/polkadot_node.hpp"
#include "storage/trie/serialization/buffer_stream.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using namespace kagome;
using namespace common;
using namespace storage;
using namespace trie;
using namespace testing;

struct NodeDecodingTest
    : public ::testing::TestWithParam<std::shared_ptr<PolkadotNode>> {

  std::unique_ptr<PolkadotCodec> codec = std::make_unique<PolkadotCodec>();
};

TEST_P(NodeDecodingTest, GetHeader) {
  auto node = GetParam();

  EXPECT_OUTCOME_TRUE(encoded, codec->encodeNode(*node));
  EXPECT_OUTCOME_TRUE(decoded, codec->decodeNode(encoded));
  auto decoded_node = std::dynamic_pointer_cast<PolkadotNode>(decoded);
  EXPECT_EQ(decoded_node->key_nibbles, node->key_nibbles);
  EXPECT_EQ(decoded_node->value, node->value);
}

template <typename T>
std::shared_ptr<PolkadotNode> make(const common::Buffer &key_nibbles,
                                   const common::Buffer &value) {
  auto node = std::make_shared<T>();
  node->key_nibbles = key_nibbles;
  node->value = value;
  return node;
}

std::shared_ptr<PolkadotNode> branch_with_2_children = []() {
  auto node = std::make_shared<BranchNode>("010203"_hex2buf, "0a"_hex2buf);
  auto child1 = std::make_shared<LeafNode>("01"_hex2buf, "0b"_hex2buf);
  auto child2 = std::make_shared<LeafNode>("02"_hex2buf, "0c"_hex2buf);
  node->children[0] = child1;
  node->children[1] = child2;
  return node;
}();

using T = PolkadotNode::Type;

static const std::vector<std::shared_ptr<PolkadotNode>> CASES = {
    make<LeafNode>("010203"_hex2buf, "abcdef"_hex2buf),
    make<LeafNode>("0a0b0c"_hex2buf, "abcdef"_hex2buf),
    make<BranchNode>("010203"_hex2buf, "abcdef"_hex2buf),
    branch_with_2_children
};

INSTANTIATE_TEST_CASE_P(PolkadotCodec, NodeDecodingTest, ValuesIn(CASES));
