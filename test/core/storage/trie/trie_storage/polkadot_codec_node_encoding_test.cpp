/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <memory>

#include <gtest/gtest.h>
#include "storage/trie/polkadot_trie/trie_node.hpp"
#include "storage/trie/serialization/polkadot_codec.hpp"
#include "testutil/outcome.hpp"

using namespace kagome;
using namespace common;
using namespace storage;
using namespace trie;
using namespace testing;

struct Case {
  std::shared_ptr<TrieNode> node;
  Buffer encoded;
};

struct NodeEncodingTest : public ::testing::TestWithParam<Case> {
  std::unique_ptr<PolkadotCodec> codec = std::make_unique<PolkadotCodec>();
};

TEST_P(NodeEncodingTest, GetHeader) {
  auto [node, expected] = GetParam();

  EXPECT_OUTCOME_TRUE_2(
      actual, codec->encodeHeader(*node, storage::trie::StateVersion::V0));
  EXPECT_EQ(actual.toHex(), expected.toHex());
}

template <typename T>
std::shared_ptr<TrieNode> make(const common::Buffer &key_nibbles,
                               std::optional<common::Buffer> value) {
  auto node = std::make_shared<T>();
  node->setKeyNibbles(key_nibbles);
  node->getMutableValue().value = value;
  return node;
}

using T = TrieNode::Type;

constexpr uint8_t LEAF = (uint8_t)T::Leaf << 6u;
constexpr uint8_t BRANCH_VAL = (uint8_t)T::BranchWithValue << 6u;
constexpr uint8_t BRANCH_NO_VAL = (uint8_t)T::BranchEmptyValue << 6u;

static const std::vector<Case> CASES = {
    {make<LeafNode>(Buffer(64, 0xf), Buffer{0x01}), {LEAF | 63u, 1}},
    {make<LeafNode>(Buffer(318, 0xf), Buffer{0x01}), {LEAF | 63u, 255, 0}},
    {make<LeafNode>(Buffer(573, 0xf), Buffer{0x01}), {LEAF | 63u, 255, 255, 0}},

    {make<BranchNode>({}, std::nullopt), {BRANCH_NO_VAL}},
    {make<BranchNode>({0}, std::nullopt), {BRANCH_NO_VAL | 1u}},
    {make<BranchNode>({0, 0, 0xf, 0x3}, std::nullopt), {BRANCH_NO_VAL | 4u}},
    {make<BranchNode>({}, Buffer{0x01}), {BRANCH_VAL}},
    {make<BranchNode>({0}, Buffer{0x01}), {BRANCH_VAL | 1u}},
    {make<BranchNode>({0, 0}, Buffer{0x01}), {BRANCH_VAL | 2u}},
    {make<BranchNode>({0, 0, 0xf}, Buffer{0x01}), {BRANCH_VAL | 3u}},
    {make<BranchNode>(Buffer(62, 0xf), std::nullopt), {BRANCH_NO_VAL | 62u}},
    {make<BranchNode>(Buffer(62, 0xf), Buffer{0x01}), {BRANCH_VAL | 62u}},
    {make<BranchNode>(Buffer(63, 0xf), std::nullopt), {BRANCH_NO_VAL | 63u, 0}},
    {make<BranchNode>(Buffer(64, 0xf), std::nullopt), {BRANCH_NO_VAL | 63u, 1}},
    {make<BranchNode>(Buffer(64, 0xf), Buffer{0x01}), {BRANCH_VAL | 63u, 1}},
    {make<BranchNode>(Buffer(317, 0xf), Buffer{0x01}), {BRANCH_VAL | 63u, 254}},
    {make<BranchNode>(Buffer(318, 0xf), Buffer{0x01}),
     {BRANCH_VAL | 63u, 255, 0}},
    {make<BranchNode>(Buffer(573, 0xf), Buffer{0x01}),
     {BRANCH_VAL | 63u, 255, 255, 0}},
};

INSTANTIATE_TEST_SUITE_P(PolkadotCodec, NodeEncodingTest, ValuesIn(CASES));
