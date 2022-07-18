/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/state_protocol_observer_impl.hpp"

#include <gtest/gtest.h>

#include "mock/core/blockchain/block_header_repository_mock.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "storage/trie/impl/trie_storage_impl.hpp"
#include "storage/trie/impl/trie_storage_backend_impl.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_factory_impl.hpp"
#include "storage/trie/serialization/trie_serializer_impl.hpp"
#include "storage/trie/trie_storage_backend.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace kagome;
using namespace network;
using namespace blockchain;
using namespace storage;
using namespace trie;

std::shared_ptr<TrieStorage> makeEmptyInMemoryTrie() {
  auto backend =
      std::make_shared<kagome::storage::trie::TrieStorageBackendImpl>(
          std::make_shared<kagome::storage::InMemoryStorage>(),
          kagome::common::Buffer{});

  auto trie_factory = std::make_shared<PolkadotTrieFactoryImpl>();
  auto codec = std::make_shared<PolkadotCodec>();
  auto serializer =
      std::make_shared<TrieSerializerImpl>(trie_factory, codec, backend);

  return kagome::storage::trie::TrieStorageImpl::createEmpty(
             trie_factory, codec, serializer, std::nullopt)
      .value();
}

class StateProtocolObserverTest : public testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    state_protocol_observer_ =
        std::make_shared<StateProtocolObserverImpl>(headers_, trie_);
  }

  std::shared_ptr<BlockHeaderRepositoryMock> headers_ =
      std::make_shared<BlockHeaderRepositoryMock>();
  std::shared_ptr<TrieStorage> trie_ = makeEmptyInMemoryTrie();

  std::shared_ptr<StateProtocolObserver> state_protocol_observer_;
};

TEST_F(StateProtocolObserverTest, Simple) {
  ;
}
