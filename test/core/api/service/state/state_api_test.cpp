/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "api/service/state/impl/state_api_impl.hpp"
#include "mock/core/blockchain/block_header_repository_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/runtime/core_mock.hpp"
#include "mock/core/api/service/state/state_api_mock.hpp"
#include "mock/core/storage/trie/trie_batches_mock.hpp"
#include "mock/core/storage/trie/trie_storage_mock.hpp"
#include "api/service/state/requests/subscribe_storage.hpp"
#include "primitives/block_header.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using kagome::blockchain::BlockHeaderRepositoryMock;
using kagome::blockchain::BlockTreeMock;
using kagome::common::Buffer;
using kagome::primitives::BlockHash;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockInfo;
using kagome::api::StateApiMock;
using kagome::runtime::CoreMock;
using kagome::storage::trie::EphemeralTrieBatchMock;
using kagome::storage::trie::TrieStorageMock;
using testing::_;
using testing::Return;

/**
 * @given state api
 * @when get a storage value for the given key (and optionally state root)
 * @then the correct value is returned
 */
TEST(StateApiTest, GetStorage) {
  auto storage = std::make_shared<TrieStorageMock>();
  auto block_header_repo = std::make_shared<BlockHeaderRepositoryMock>();
  auto block_tree = std::make_shared<BlockTreeMock>();
  auto runtime_core = std::make_shared<CoreMock>();

  kagome::api::StateApiImpl api{
      block_header_repo, storage, block_tree, runtime_core, nullptr};

  EXPECT_CALL(*block_tree, getLastFinalized())
      .WillOnce(testing::Return(BlockInfo(42, "D"_hash256)));
  kagome::primitives::BlockId did = "D"_hash256;
  EXPECT_CALL(*block_header_repo, getBlockHeader(did))
      .WillOnce(testing::Return(BlockHeader{.state_root = "CDE"_hash256}));
  EXPECT_CALL(*storage, getEphemeralBatchAt(_))
      .WillRepeatedly(testing::Invoke([](auto &root) {
        auto batch = std::make_unique<EphemeralTrieBatchMock>();
        EXPECT_CALL(*batch, get("a"_buf))
            .WillRepeatedly(testing::Return("1"_buf));
        return batch;
      }));

  EXPECT_OUTCOME_TRUE(r, api.getStorage("a"_buf));
  ASSERT_EQ(r, "1"_buf);

  kagome::primitives::BlockId bid = "B"_hash256;
  EXPECT_CALL(*block_header_repo, getBlockHeader(bid))
      .WillOnce(testing::Return(BlockHeader{.state_root = "ABC"_hash256}));

  EXPECT_OUTCOME_TRUE(r1, api.getStorage("a"_buf, "B"_hash256));
  ASSERT_EQ(r1, "1"_buf);
}

/**
 * @given state api
 * @when get a runtime version for the given block hash
 * @then the correct value is returned
 */
TEST(StateApiTest, GetRuntimeVersion) {
  auto storage = std::make_shared<TrieStorageMock>();
  auto block_header_repo = std::make_shared<BlockHeaderRepositoryMock>();
  auto block_tree = std::make_shared<BlockTreeMock>();
  auto runtime_core = std::make_shared<CoreMock>();

  kagome::api::StateApiImpl api{
      block_header_repo, storage, block_tree, runtime_core, nullptr};

  kagome::primitives::Version test_version{.spec_name = "dummy_sn",
                                           .impl_name = "dummy_in",
                                           .authoring_version = 0x101,
                                           .spec_version = 0x111,
                                           .impl_version = 0x202};

  boost::optional<kagome::primitives::BlockHash> hash1 = boost::none;
  EXPECT_CALL(*runtime_core, version(hash1))
      .WillOnce(testing::Return(test_version));

  {
    EXPECT_OUTCOME_TRUE(result, api.getRuntimeVersion(boost::none));
    ASSERT_EQ(result, test_version);
  }

  boost::optional<kagome::primitives::BlockHash> hash = "T"_hash256;
  EXPECT_CALL(*runtime_core, version(hash))
      .WillOnce(testing::Return(test_version));

  {
    EXPECT_OUTCOME_TRUE(result, api.getRuntimeVersion("T"_hash256));
    ASSERT_EQ(result, test_version);
  }
}

/**
 * @given state api
 * @when call a subscribe storage with a given set on keys
 * @then the correct values are returned
 */
TEST(StateApiTest, SubscribeStorage) {
  auto state_api = std::make_shared<StateApiMock>();
  auto subscribe_storage = std::make_shared<kagome::api::state::request::SubscribeStorage>(state_api);

  std::vector<Buffer> keys = {
      { 0x10, 0x11, 0x12, 0x13 },
      { 0x50, 0x51, 0x52, 0x53 },
  };

  EXPECT_CALL(*state_api, subscribeStorage(keys))
      .WillOnce(testing::Return(55));

  jsonrpc::Request::Parameters params;
  params.push_back(jsonrpc::Value::Array{ std::string("0x") + keys[0].toHex(), std::string("0x") + keys[1].toHex()});

  subscribe_storage->init(params);
  EXPECT_OUTCOME_TRUE(result, subscribe_storage->execute());
  ASSERT_EQ(result, 55);
}

/**
 * @given state api
 * @when call a subscribe storage with a given BAD key
 * @then we skip processing and return error
 */
TEST(StateApiTest, SubscribeStorageInvalidData) {
  auto state_api = std::make_shared<StateApiMock>();
  auto subscribe_storage = std::make_shared<kagome::api::state::request::SubscribeStorage>(state_api);

  jsonrpc::Request::Parameters params;
  params.push_back(jsonrpc::Value::Array{ std::string("test_data") });

  EXPECT_OUTCOME_ERROR(result, subscribe_storage->init(params), kagome::common::UnhexError::MISSING_0X_PREFIX);
}

/**
 * @given state api
 * @when call a subscribe storage with a given BAD key
 * @then we skip processing and return error
 */
TEST(StateApiTest, SubscribeStorageWithoutPrefix) {
  auto state_api = std::make_shared<StateApiMock>();
  auto subscribe_storage = std::make_shared<kagome::api::state::request::SubscribeStorage>(state_api);

  jsonrpc::Request::Parameters params;
  params.push_back(jsonrpc::Value::Array{ std::string("aa1122334455") });

  EXPECT_OUTCOME_ERROR(result, subscribe_storage->init(params), kagome::common::UnhexError::MISSING_0X_PREFIX);
}

/**
 * @given state api
 * @when call a subscribe storage with a given BAD key
 * @then we skip processing and return error
 */
TEST(StateApiTest, SubscribeStorageBadBoy) {
  auto state_api = std::make_shared<StateApiMock>();
  auto subscribe_storage = std::make_shared<kagome::api::state::request::SubscribeStorage>(state_api);

  jsonrpc::Request::Parameters params;
  params.push_back(jsonrpc::Value::Array{ std::string("0xtest_data") });

  EXPECT_OUTCOME_ERROR(result, subscribe_storage->init(params), kagome::common::UnhexError::NON_HEX_INPUT);
}
