/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <memory>

#include <gtest/gtest.h>
#include "storage/trie/polkadot_trie/polkadot_node.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"
#include "testutil/outcome.hpp"

using namespace kagome;
using namespace common;
using namespace storage;
using namespace trie;
using namespace testing;

struct Case {
  std::shared_ptr<PolkadotNode> node;
  Buffer encoded;
};

struct NodeEncodingTest : public ::testing::TestWithParam<Case> {
  std::unique_ptr<PolkadotCodec> codec = std::make_unique<PolkadotCodec>();
};

TEST_P(NodeEncodingTest, GetHeader) {
  auto [node, expected] = GetParam();

  EXPECT_OUTCOME_TRUE_2(actual, codec->encodeHeader(*node));
  EXPECT_EQ(actual.toHex(), expected.toHex());
}

template <typename T>
std::shared_ptr<PolkadotNode> make(const common::Buffer &key_nibbles,
                                   boost::optional<common::Buffer> value) {
  auto node = std::make_shared<T>();
  node->key_nibbles = key_nibbles;
  node->value = value;
  return node;
}

using T = PolkadotNode::Type;

constexpr uint8_t LEAF = (uint8_t)T::Leaf << 6u;
constexpr uint8_t BRANCH_VAL = (uint8_t)T::BranchWithValue << 6u;
constexpr uint8_t BRANCH_NO_VAL = (uint8_t)T::BranchEmptyValue << 6u;

static const std::vector<Case> CASES = {
    {make<LeafNode>({}, boost::none), {LEAF}},                         // 0
    {make<LeafNode>({0}, boost::none), {LEAF | 1u}},                   // 1
    {make<LeafNode>({0, 0, 0xf, 0x3}, boost::none), {LEAF | 4u}},      // 2
    {make<LeafNode>(Buffer(62, 0xf), boost::none), {LEAF | 62u}},      // 3
    {make<LeafNode>(Buffer(63, 0xf), boost::none), {LEAF | 63u, 0}},   // 4
    {make<LeafNode>(Buffer(64, 0xf), Buffer{0x01}), {LEAF | 63u, 1}},  // 5
    {make<LeafNode>(Buffer(318, 0xf), Buffer{0x01}),
     {LEAF | 63u, 255, 0}},  // 6
    {make<LeafNode>(Buffer(573, 0xf), Buffer{0x01}),
     {LEAF | 63u, 255, 255, 0}},  // 7

    {make<BranchNode>({}, boost::none), {BRANCH_NO_VAL}},        // 8
    {make<BranchNode>({0}, boost::none), {BRANCH_NO_VAL | 1u}},  // 9
    {make<BranchNode>({0, 0, 0xf, 0x3}, boost::none),
     {BRANCH_NO_VAL | 4u}},                                            // 10
    {make<BranchNode>({}, Buffer{0x01}), {BRANCH_VAL}},                // 11
    {make<BranchNode>({0}, Buffer{0x01}), {BRANCH_VAL | 1u}},          // 12
    {make<BranchNode>({0, 0}, Buffer{0x01}), {BRANCH_VAL | 2u}},       // 13
    {make<BranchNode>({0, 0, 0xf}, Buffer{0x01}), {BRANCH_VAL | 3u}},  // 14
    {make<BranchNode>(Buffer(62, 0xf), boost::none),
     {BRANCH_NO_VAL | 62u}},  // 15
    {make<BranchNode>(Buffer(62, 0xf), Buffer{0x01}),
     {BRANCH_VAL | 62u}},  // 16
    {make<BranchNode>(Buffer(63, 0xf), boost::none),
     {BRANCH_NO_VAL | 63u, 0}},  // 17
    {make<BranchNode>(Buffer(64, 0xf), boost::none),
     {BRANCH_NO_VAL | 63u, 1}},  // 18
    {make<BranchNode>(Buffer(64, 0xf), Buffer{0x01}),
     {BRANCH_VAL | 63u, 1}},  // 19
    {make<BranchNode>(Buffer(317, 0xf), Buffer{0x01}),
     {BRANCH_VAL | 63u, 254}},  // 20
    {make<BranchNode>(Buffer(318, 0xf), Buffer{0x01}),
     {BRANCH_VAL | 63u, 255, 0}},  // 21
    {make<BranchNode>(Buffer(573, 0xf), Buffer{0x01}),
     {BRANCH_VAL | 63u, 255, 255, 0}},  // 22
};

INSTANTIATE_TEST_CASE_P(PolkadotCodec, NodeEncodingTest, ValuesIn(CASES));
