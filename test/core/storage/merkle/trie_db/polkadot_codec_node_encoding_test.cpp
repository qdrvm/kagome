/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <memory>

#include <gtest/gtest.h>
#include "scale/buffer_codec.hpp"
#include "storage/merkle/polkadot_trie_db/polkadot_codec.hpp"
#include "storage/merkle/polkadot_trie_db/polkadot_node.hpp"
#include "testutil/outcome.hpp"

using namespace kagome;
using namespace common;
using namespace scale;
using namespace storage;
using namespace merkle;
using namespace testing;

struct Case {
  std::shared_ptr<PolkadotNode> node;
  Buffer encoded;
};

struct NodeEncodingTest : public ::testing::TestWithParam<Case> {
  std::shared_ptr<BufferScaleCodec> scale =
      std::make_shared<BufferScaleCodec>();

  std::unique_ptr<PolkadotCodec> codec = std::make_unique<PolkadotCodec>(scale);
};

TEST_P(NodeEncodingTest, GetHeader) {
  auto [node, expected] = GetParam();

  EXPECT_OUTCOME_TRUE_2(actual, codec->getHeader(*node));
  EXPECT_EQ(actual.toHex(), expected.toHex());
}

template <typename T>
std::shared_ptr<PolkadotNode> make(const common::Buffer &key_nibbles,
                                   const common::Buffer &value) {
  auto node = std::make_shared<T>();
  node->key_nibbles = key_nibbles;
  node->value = value;
  return node;
}

static const std::vector<Case> CASES = {
    /// https://sourcegraph.com/github.com/ChainSafe/gossamer/-/blob/trie/node_test.go#L100
    {make<LeafNode>({}, {}), {1}},                                   // 0
    {make<LeafNode>({0}, {}), {5}},                                  // 1
    {make<LeafNode>({0, 0, 0xf, 0x3}, {}), {17}},                    // 2
    {make<LeafNode>(Buffer(62, 0xf), {}), {0xf9}},                   // 3
    {make<LeafNode>(Buffer(63, 0xf), {}), {253, 0}},                 // 4
    {make<LeafNode>(Buffer(64, 0xf), {0x01}), {253, 1}},             // 5
    {make<LeafNode>(Buffer(318, 0xf), {0x01}), {253, 255, 0}},       // 6
    {make<LeafNode>(Buffer(573, 0xf), {0x01}), {253, 255, 255, 0}},  // 7

    /// https://sourcegraph.com/github.com/ChainSafe/gossamer/-/blob/trie/node_test.go#L67
    {make<BranchNode>({}, {}), {2}},                                   // 8
    {make<BranchNode>({0}, {}), {6}},                                  // 9
    {make<BranchNode>({0, 0, 0xf, 0x3}, {}), {18}},                    // 10
    {make<BranchNode>({}, {0x01}), {3}},                               // 11
    {make<BranchNode>({0}, {0x01}), {7}},                              // 12
    {make<BranchNode>({0, 0}, {0x01}), {11}},                          // 13
    {make<BranchNode>({0, 0, 0xf}, {0x01}), {15}},                     // 14
    {make<BranchNode>(Buffer(62, 0xf), {}), {0xfa}},                   // 15
    {make<BranchNode>(Buffer(62, 0xf), {0x01}), {0xfb}},               // 16
    {make<BranchNode>(Buffer(63, 0xf), {}), {254, 0}},                 // 17
    {make<BranchNode>(Buffer(64, 0xf), {}), {254, 1}},                 // 18
    {make<BranchNode>(Buffer(64, 0xf), {0x01}), {255, 1}},             // 19
    {make<BranchNode>(Buffer(317, 0xf), {0x01}), {255, 254}},          // 20
    {make<BranchNode>(Buffer(318, 0xf), {0x01}), {255, 255, 0}},       // 21
    {make<BranchNode>(Buffer(573, 0xf), {0x01}), {255, 255, 255, 0}},  // 22
};

INSTANTIATE_TEST_CASE_P(PolkadotCodec, NodeEncodingTest, ValuesIn(CASES));
