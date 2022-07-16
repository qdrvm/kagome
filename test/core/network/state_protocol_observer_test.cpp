/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/state_protocol_observer_impl.hpp"

#include <gtest/gtest.h>

#include "mock/core/blockchain/block_header_repository_mock.hpp"
#include "mock/core/storage/trie/trie_storage_mock.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace kagome;
using namespace network;
using namespace blockchain;
using namespace storage;
using namespace trie;

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
  std::shared_ptr<TrieStorageMock> trie_ = std::make_shared<TrieStorageMock>();

  std::shared_ptr<StateProtocolObserver> state_protocol_observer_;
};

TEST_F(StateProtocolObserverTest, Simple) {

}
