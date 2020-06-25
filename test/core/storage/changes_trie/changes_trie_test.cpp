/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/changes_trie/impl/changes_trie.hpp"

#include <gtest/gtest.h>

#include "mock/core/storage/trie/trie_storage_mock.hpp"
#include "storage/changes_trie/impl/storage_changes_tracker_impl.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_factory_impl.hpp"
#include "testutil/literals.hpp"

using kagome::common::Buffer;
using kagome::storage::changes_trie::ChangesTrie;
using kagome::storage::changes_trie::ChangesTrieConfig;
using kagome::storage::trie::PolkadotCodec;
using kagome::storage::trie::PolkadotTrieFactoryImpl;
using kagome::storage::trie::TrieStorageMock;
using testing::_;
using testing::Return;

/**
 * @given a changes trie with congifuration identical to a one in a substrate
 * test
 * @when calculationg its hash
 * @then it matches the hash from substrate
 */
TEST(ChangesTrieTest, SubstrateCompatibility) {
  auto factory = std::make_shared<PolkadotTrieFactoryImpl>();
  auto codec = std::make_shared<PolkadotCodec>();

  auto storage_mock = std::make_shared<TrieStorageMock>();

  std::map<Buffer, std::vector<kagome::primitives::ExtrinsicIndex>> changes{
      {Buffer{1}, {1}}, {":extrinsic_index"_buf, {1}}};

  auto changes_trie = ChangesTrie::buildFromChanges(
                          99, factory, codec, changes, ChangesTrieConfig{})
                          .value();
  auto hash = changes_trie->getHash();
  ASSERT_EQ(
      hash,
      kagome::common::Hash256::fromHex(
          "bb0c2ef6e1d36d5490f9766cfcc7dfe2a6ca804504c3bb206053890d6dd02376")
          .value());
}
