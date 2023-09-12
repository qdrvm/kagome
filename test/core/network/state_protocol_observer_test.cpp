/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/state_protocol_observer_impl.hpp"

#include <gtest/gtest.h>
#include <cstdint>

#include "mock/core/blockchain/block_header_repository_mock.hpp"
#include "mock/core/storage/trie_pruner/trie_pruner_mock.hpp"
#include "network/types/state_request.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "storage/trie/impl/trie_storage_backend_impl.hpp"
#include "storage/trie/impl/trie_storage_impl.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_factory_impl.hpp"
#include "storage/trie/serialization/trie_serializer_impl.hpp"
#include "storage/trie/trie_batches.hpp"
#include "storage/trie/trie_storage_backend.hpp"
#include "storage/trie/types.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace kagome;

using namespace blockchain;
using namespace common;
using namespace network;
using namespace primitives;
using namespace storage;

using namespace trie;
using namespace trie_pruner;

using testing::_;
using testing::Return;

std::shared_ptr<TrieStorage> makeEmptyInMemoryTrie() {
  auto backend =
      std::make_shared<kagome::storage::trie::TrieStorageBackendImpl>(
          std::make_shared<kagome::storage::InMemoryStorage>());

  auto trie_factory = std::make_shared<PolkadotTrieFactoryImpl>();
  auto codec = std::make_shared<PolkadotCodec>();
  auto serializer =
      std::make_shared<TrieSerializerImpl>(trie_factory, codec, backend);
  auto state_pruner = std::make_shared<TriePrunerMock>();
  ON_CALL(*state_pruner,
          addNewState(testing::A<const storage::trie::PolkadotTrie &>(), _))
      .WillByDefault(Return(outcome::success()));

  return kagome::storage::trie::TrieStorageImpl::createEmpty(
             trie_factory, codec, serializer, state_pruner)
      .value();
}

Hash256 makeHash(std::string_view str) {
  kagome::common::Hash256 hash{};
  std::copy_n(str.begin(), std::min(str.size(), 32ul), hash.rbegin());
  return hash;
}

BlockHeader makeBlockHeader(RootHash hash) {
  uint32_t num = 1;
  std::string str_num = std::to_string(num);
  return kagome::primitives::BlockHeader{
      makeHash("block_genesis_hash"),
      num,
      hash,
      makeHash("block_" + str_num + "_ext_root"),
      {}};
}

class StateProtocolObserverTest : public testing::Test {
 protected:
  outcome::result<std::unique_ptr<TrieBatch>> persistent_empty_batch() {
    auto codec = std::make_shared<PolkadotCodec>();
    OUTCOME_TRY(batch,
                trie_->getPersistentBatchAt(kEmptyRootHash, std::nullopt));
    return batch;
  }

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

namespace kagome::network {
  bool operator==(const StateEntry &lhs, const StateEntry &rhs) {
    return lhs.key == rhs.key && lhs.value == rhs.value;
  }

  bool operator==(const KeyValueStateEntry &lhs,
                  const KeyValueStateEntry &rhs) {
    return lhs.state_root == rhs.state_root && lhs.entries == rhs.entries
        && lhs.complete == rhs.complete;
  }

  bool operator==(const StateResponse &lhs, const StateResponse &rhs) {
    return lhs.entries == rhs.entries && lhs.proof == rhs.proof;
  }
}  // namespace kagome::network

/**
 * @given trie state with 2 keys
 * @when default state request
 * @then response with 2 entries
 */
TEST_F(StateProtocolObserverTest, Simple) {
  EXPECT_OUTCOME_TRUE(batch, persistent_empty_batch());
  std::ignore = batch->put("abc"_buf, "123"_buf);
  std::ignore = batch->put("cde"_buf, "345"_buf);
  EXPECT_OUTCOME_TRUE(hash, batch->commit(storage::trie::StateVersion::V0));

  auto header = makeBlockHeader(hash);
  EXPECT_CALL(*headers_, getBlockHeader({"1"_hash256}))
      .WillRepeatedly(testing::Return(header));

  StateRequest request{
      .hash = "1"_hash256,
      .start = {},
      .no_proof = true,
  };

  EXPECT_OUTCOME_TRUE(response,
                      state_protocol_observer_->onStateRequest(request));

  StateResponse ref = {
      .entries = {{
          .state_root = {},
          .entries = {{.key = "abc"_buf, .value = "123"_buf},
                      {.key = "cde"_buf, .value = "345"_buf}},
          .complete = true,
      }},
  };

  ASSERT_EQ(response, ref);
}
