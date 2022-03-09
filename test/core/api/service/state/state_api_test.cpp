/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "api/service/state/impl/state_api_impl.hpp"
#include "api/service/state/requests/get_metadata.hpp"
#include "api/service/state/requests/subscribe_storage.hpp"
#include "core/storage/trie/polkadot_trie_cursor_dummy.hpp"
#include "mock/core/api/service/api_service_mock.hpp"
#include "mock/core/api/service/state/state_api_mock.hpp"
#include "mock/core/blockchain/block_header_repository_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/runtime/core_mock.hpp"
#include "mock/core/runtime/metadata_mock.hpp"
#include "mock/core/storage/trie/trie_batches_mock.hpp"
#include "mock/core/storage/trie/trie_storage_mock.hpp"
#include "primitives/block_header.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using kagome::api::ApiServiceMock;
using kagome::api::StateApiMock;
using kagome::blockchain::BlockHeaderRepositoryMock;
using kagome::blockchain::BlockTreeMock;
using kagome::common::Buffer;
using kagome::primitives::BlockHash;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockInfo;
using kagome::runtime::CoreMock;
using kagome::runtime::MetadataMock;
using kagome::storage::trie::EphemeralTrieBatchMock;
using kagome::storage::trie::TrieStorageMock;
using testing::_;
using testing::ElementsAre;
using testing::Return;

namespace kagome::api {

  class StateApiTest : public ::testing::Test {
   public:
    void SetUp() override {
      api_ = std::make_unique<api::StateApiImpl>(
          block_header_repo_, storage_, block_tree_, runtime_core_, metadata_);
    }

   protected:
    std::shared_ptr<TrieStorageMock> storage_ =
        std::make_shared<TrieStorageMock>();
    std::shared_ptr<BlockHeaderRepositoryMock> block_header_repo_ =
        std::make_shared<BlockHeaderRepositoryMock>();
    std::shared_ptr<BlockTreeMock> block_tree_ =
        std::make_shared<BlockTreeMock>();
    std::shared_ptr<CoreMock> runtime_core_ = std::make_shared<CoreMock>();
    std::shared_ptr<MetadataMock> metadata_ = std::make_shared<MetadataMock>();
    std::shared_ptr<ApiServiceMock> api_service_ =
        std::make_shared<ApiServiceMock>();

    std::unique_ptr<api::StateApiImpl> api_{};
  };

  /**
   * @given state api
   * @when get a storage value for the given key (and optionally state root)
   * @then the correct value is returned
   */
  TEST_F(StateApiTest, GetStorage) {
    EXPECT_CALL(*block_tree_, getLastFinalized())
        .WillOnce(testing::Return(BlockInfo(42, "D"_hash256)));
    primitives::BlockId did = "D"_hash256;
    EXPECT_CALL(*block_header_repo_, getBlockHeader(did))
        .WillOnce(testing::Return(BlockHeader{.state_root = "CDE"_hash256}));
    auto in_buf = "a"_buf;
    auto out_buf = "1"_buf;
    EXPECT_CALL(*storage_, getEphemeralBatchAt(_))
        .WillRepeatedly(testing::Invoke([&in_buf, &out_buf](auto &root) {
          auto batch = std::make_unique<EphemeralTrieBatchMock>();
          EXPECT_CALL(*batch, tryGet(in_buf.view()))
              .WillRepeatedly(testing::Return(std::cref(out_buf)));
          return batch;
        }));

    auto key = "a"_buf;
    EXPECT_OUTCOME_TRUE(r, api_->getStorage(key.view()))
    ASSERT_EQ(r.value().get(), "1"_buf);

    primitives::BlockId bid = "B"_hash256;
    EXPECT_CALL(*block_header_repo_, getBlockHeader(bid))
        .WillOnce(testing::Return(BlockHeader{.state_root = "ABC"_hash256}));

    EXPECT_OUTCOME_TRUE(r1, api_->getStorageAt(key.view(), "B"_hash256));
    ASSERT_EQ(r1.value().get(), "1"_buf);
  }

  class GetKeysPagedTest : public ::testing::Test {
   public:
    void SetUp() override {
      auto storage = std::make_shared<TrieStorageMock>();
      block_header_repo_ = std::make_shared<BlockHeaderRepositoryMock>();
      block_tree_ = std::make_shared<BlockTreeMock>();
      auto runtime_core = std::make_shared<CoreMock>();
      auto metadata = std::make_shared<MetadataMock>();

      api_ = std::make_shared<api::StateApiImpl>(
          block_header_repo_, storage, block_tree_, runtime_core, metadata);

      EXPECT_CALL(*block_tree_, getLastFinalized())
          .WillOnce(testing::Return(BlockInfo(42, "D"_hash256)));
      primitives::BlockId did = "D"_hash256;

      EXPECT_CALL(*block_header_repo_, getBlockHeader(did))
          .WillOnce(testing::Return(BlockHeader{.state_root = "CDE"_hash256}));

      EXPECT_CALL(*storage, getEphemeralBatchAt(_))
          .WillRepeatedly(testing::Invoke([this](auto &root) {
            auto batch = std::make_unique<EphemeralTrieBatchMock>();
            EXPECT_CALL(*batch, trieCursor())
                .WillRepeatedly(testing::Invoke([this]() {
                  return std::make_unique<
                      storage::trie::PolkadotTrieCursorDummy>(lex_sorted_vals);
                }));
            return batch;
          }));
    }

   protected:
    std::shared_ptr<BlockHeaderRepositoryMock> block_header_repo_;
    std::shared_ptr<BlockTreeMock> block_tree_;
    std::shared_ptr<api::StateApiImpl> api_;

    const std::map<Buffer, Buffer, std::less<>> lex_sorted_vals{
        {"0102"_hex2buf, "0102"_hex2buf},
        {"0103"_hex2buf, "0103"_hex2buf},
        {"010304"_hex2buf, "010304"_hex2buf},
        {"05"_hex2buf, "05"_hex2buf},
        {"06"_hex2buf, "06"_hex2buf},
        {"0607"_hex2buf, "0607"_hex2buf},
        {"060708"_hex2buf, "060708"_hex2buf},
        {"06070801"_hex2buf, "06070801"_hex2buf},
        {"06070802"_hex2buf, "06070802"_hex2buf},
        {"06070803"_hex2buf, "06070803"_hex2buf},
        {"07"_hex2buf, "07"_hex2buf}};
  };

  /**
   * @given state api with cursor over predefined set of key-vals
   * @when getKeysPaged invoked with no prefix
   * @then expected amount of keys from beginning of cursor are returned
   */
  TEST_F(GetKeysPagedTest, EmptyParamsTest) {
    EXPECT_OUTCOME_TRUE(
        val, api_->getKeysPaged(std::nullopt, 2, std::nullopt, std::nullopt));
    ASSERT_THAT(val, ElementsAre("0102"_hex2buf, "0103"_hex2buf));
  }

  /**
   * @given state api with cursor over predefined set of key-vals
   * @when getKeysPaged invoked with prefix
   * @then expected amount of keys with provided prefix are returned
   */
  TEST_F(GetKeysPagedTest, NonEmptyPrefixTest) {
    EXPECT_OUTCOME_TRUE(
        val, api_->getKeysPaged("0607"_hex2buf, 3, std::nullopt, std::nullopt));
    ASSERT_THAT(
        val, ElementsAre("0607"_hex2buf, "060708"_hex2buf, "06070801"_hex2buf));
  }

  /**
   * @given state api with cursor over predefined set of key-vals
   * @when getKeysPaged invoked with prefix and prev_key
   * @then exepected amount of keys after provided prev_key are returned
   */
  TEST_F(GetKeysPagedTest, NonEmptyPrevKeyTest) {
    EXPECT_OUTCOME_TRUE(
        val, api_->getKeysPaged("06"_hex2buf, 3, "0607"_hex2buf, std::nullopt));
    ASSERT_THAT(
        val,
        ElementsAre("060708"_hex2buf, "06070801"_hex2buf, "06070802"_hex2buf));
  }

  /**
   * @given state api with cursor over predefined set of key-vals
   * @when getKeysPaged invoked with non-empty prev_key and non-empty prefix
   * that is bigger than prev_key
   * @then expected amount of keys with provided prefix after prev_key are
   * returned
   */
  TEST_F(GetKeysPagedTest, PrefixBiggerThanPrevkey) {
    EXPECT_OUTCOME_TRUE(
        val,
        api_->getKeysPaged("060708"_hex2buf, 5, "06"_hex2buf, std::nullopt));
    ASSERT_THAT(val,
                ElementsAre("060708"_hex2buf,
                            "06070801"_hex2buf,
                            "06070802"_hex2buf,
                            "06070803"_hex2buf));
  }

  /**
   * @given state api
   * @when get a runtime version for the given block hash
   * @then the correct value is returned
   */
  TEST_F(StateApiTest, GetRuntimeVersion) {
    primitives::Version test_version{.spec_name = "dummy_sn",
                                     .impl_name = "dummy_in",
                                     .authoring_version = 0x101,
                                     .spec_version = 0x111,
                                     .impl_version = 0x202};

    EXPECT_CALL(*block_tree_, deepestLeaf())
        .WillOnce(Return(primitives::BlockInfo{42, "block42"_hash256}));
    EXPECT_CALL(*runtime_core_, version("block42"_hash256))
        .WillOnce(testing::Return(test_version));

    {
      EXPECT_OUTCOME_TRUE(result, api_->getRuntimeVersion(std::nullopt));
      ASSERT_EQ(result, test_version);
    }

    primitives::BlockHash hash = "T"_hash256;
    EXPECT_CALL(*runtime_core_, version(hash))
        .WillOnce(testing::Return(test_version));

    {
      EXPECT_OUTCOME_TRUE(result, api_->getRuntimeVersion("T"_hash256));
      ASSERT_EQ(result, test_version);
    }
  }

  /**
   * @given state api
   * @when call a subscribe storage with a given set on keys
   * @then the correct values are returned
   */
  TEST_F(StateApiTest, SubscribeStorage) {
    auto state_api = std::make_shared<StateApiMock>();
    auto subscribe_storage =
        std::make_shared<api::state::request::SubscribeStorage>(state_api);

    std::vector<Buffer> keys = {
        {0x10, 0x11, 0x12, 0x13},
        {0x50, 0x51, 0x52, 0x53},
    };

    EXPECT_CALL(*state_api, subscribeStorage(keys))
        .WillOnce(testing::Return(55));

    jsonrpc::Request::Parameters params;
    params.push_back(
        jsonrpc::Value::Array{std::string("0x") + keys[0].toHex(),
                              std::string("0x") + keys[1].toHex()});

    EXPECT_OUTCOME_SUCCESS(r, subscribe_storage->init(params));
    EXPECT_OUTCOME_TRUE(result, subscribe_storage->execute());
    ASSERT_EQ(result, 55);
  }

  /**
   * @given state api
   * @when call a subscribe storage with a given BAD key
   * @then we skip processing and return error
   */
  TEST_F(StateApiTest, SubscribeStorageInvalidData) {
    auto state_api = std::make_shared<StateApiMock>();
    auto subscribe_storage =
        std::make_shared<api::state::request::SubscribeStorage>(state_api);

    jsonrpc::Request::Parameters params;
    params.push_back(jsonrpc::Value::Array{std::string("test_data")});

    EXPECT_OUTCOME_ERROR(result,
                         subscribe_storage->init(params),
                         common::UnhexError::MISSING_0X_PREFIX);
  }

  /**
   * @given state api
   * @when call a subscribe storage with a given BAD key
   * @then we skip processing and return error
   */
  TEST_F(StateApiTest, SubscribeStorageWithoutPrefix) {
    auto state_api = std::make_shared<StateApiMock>();
    auto subscribe_storage =
        std::make_shared<api::state::request::SubscribeStorage>(state_api);

    jsonrpc::Request::Parameters params;
    params.push_back(jsonrpc::Value::Array{std::string("aa1122334455")});

    EXPECT_OUTCOME_ERROR(result,
                         subscribe_storage->init(params),
                         common::UnhexError::MISSING_0X_PREFIX);
  }

  /**
   * @given state api
   * @when call a subscribe storage with a given BAD key
   * @then we skip processing and return error
   */
  TEST_F(StateApiTest, SubscribeStorageBadBoy) {
    auto state_api = std::make_shared<StateApiMock>();
    auto subscribe_storage =
        std::make_shared<api::state::request::SubscribeStorage>(state_api);

    jsonrpc::Request::Parameters params;
    params.push_back(jsonrpc::Value::Array{std::string("0xtest_data")});

    EXPECT_OUTCOME_ERROR(result,
                         subscribe_storage->init(params),
                         common::UnhexError::NON_HEX_INPUT);
  }

  /**
   * @given state api
   * @when call getMetadata
   * @then we receive correct data
   */
  TEST_F(StateApiTest, GetMetadata) {
    auto state_api = std::make_shared<StateApiMock>();
    auto get_metadata =
        std::make_shared<api::state::request::GetMetadata>(state_api);

    std::string data = "test_data";

    jsonrpc::Request::Parameters params;
    EXPECT_CALL(*state_api, getMetadata()).WillOnce(testing::Return(data));

    EXPECT_OUTCOME_SUCCESS(r, get_metadata->init(params));
    EXPECT_OUTCOME_TRUE(result, get_metadata->execute());
    ASSERT_EQ(result, data);
  }

  MATCHER_P(ContainedIn, container, "") {
    return ::testing::ExplainMatchResult(
        ::testing::Contains(arg), container, result_listener);
  }

  /**
   * @given that every queried key changed in every queired block
   * @when querying these changes through queryStorage
   * @then all changes are reported for every block
   */
  TEST_F(StateApiTest, QueryStorageSucceeds) {
    // GIVEN
    std::vector<common::Buffer> keys{"key1"_buf, "key2"_buf, "key3"_buf};
    primitives::BlockHash from{"from"_hash256};
    primitives::BlockHash to{"to"_hash256};

    std::vector block_range{from, "block2"_hash256, "block3"_hash256, to};
    EXPECT_CALL(*block_tree_, getChainByBlocks(from, to))
        .WillOnce(testing::Return(block_range));
    EXPECT_CALL(*block_header_repo_, getNumberByHash(from))
        .WillOnce(testing::Return(1));
    EXPECT_CALL(*block_header_repo_, getNumberByHash(to))
        .WillOnce(testing::Return(4));
    for (auto &block_hash : block_range) {
      primitives::BlockHash state_root;
      auto s = block_hash.toString() + "_etats";
      std::copy_if(s.begin(), s.end(), state_root.begin(), [](auto b) {
        return b != 0;
      });
      EXPECT_CALL(*block_header_repo_,
                  getBlockHeader(primitives::BlockId{block_hash}))
          .WillOnce(testing::Return(
              primitives::BlockHeader{.state_root = state_root}));
      EXPECT_CALL(*storage_, getEphemeralBatchAt(state_root))
          .WillOnce(testing::Invoke([&keys](auto &root) {
            auto batch =
                std::make_unique<storage::trie::EphemeralTrieBatchMock>();
            for (auto &key : keys) {
              EXPECT_CALL(*batch, tryGet(key.view()))
                  .WillOnce(testing::Return(common::Buffer(root)));
            }
            return batch;
          }));
    }
    // WHEN
    EXPECT_OUTCOME_TRUE(changes, api_->queryStorage(keys, from, to))

    // THEN
    auto current_block = block_range.begin();
    for (auto &block_changes : changes) {
      ASSERT_EQ(*current_block, block_changes.block);
      ASSERT_THAT(block_changes.changes,
                  ::testing::Each(::testing::Field(
                      &StateApiImpl::StorageChangeSet::Change::key,
                      ContainedIn(keys))));
      current_block++;
    }
  }

  /**
   * @given Block range longer than the maximum allowed block range of State API
   * @when querying storage changes for this range via queryStorage
   * @then MAX_BLOCK_RANGE_EXCEEDED error is returned
   */
  TEST_F(StateApiTest, HitsBlockRangeLimits) {
    primitives::BlockHash from{"from"_hash256}, to{"to"_hash256};
    EXPECT_CALL(*block_header_repo_, getNumberByHash(from))
        .WillOnce(Return(42));
    EXPECT_CALL(*block_header_repo_, getNumberByHash(to))
        .WillOnce(Return(42 + StateApiImpl::kMaxBlockRange + 1));
    EXPECT_OUTCOME_FALSE(
        error, api_->queryStorage(std::vector{"some_key"_buf}, from, to));
    ASSERT_EQ(error, StateApiImpl::Error::MAX_BLOCK_RANGE_EXCEEDED);
  }

  /**
   * @given Key set larger than the maximum allowed key set of State API
   * @when querying storage changes for this set via queryStorage
   * @then MAX_KEY_SET_SIZE_EXCEEDED error is returned
   */
  TEST_F(StateApiTest, HitsKeyRangeLimits) {
    std::vector<common::Buffer> keys(StateApiImpl::kMaxKeySetSize + 1);
    primitives::BlockHash from{"from"_hash256}, to{"to"_hash256};
    EXPECT_OUTCOME_FALSE(error, api_->queryStorage(keys, from, to));
    ASSERT_EQ(error, StateApiImpl::Error::MAX_KEY_SET_SIZE_EXCEEDED);
  }

  /**
   * @given that every queried key changed in the given block
   * @when querying these changes through queryStorageAt
   * @then all changes are reported for the given block
   */
  TEST_F(StateApiTest, QueryStorageAtSucceeds) {
    // GIVEN
    std::vector<common::Buffer> keys{"key1"_buf, "key2"_buf, "key3"_buf};
    primitives::BlockHash at{"at"_hash256};
    std::vector block_range{at};

    EXPECT_CALL(*block_tree_, getChainByBlocks(at, at))
        .WillOnce(testing::Return(block_range));

    primitives::BlockHash state_root = "at_state"_hash256;
    EXPECT_CALL(*block_header_repo_, getBlockHeader(primitives::BlockId{at}))
        .WillOnce(
            testing::Return(primitives::BlockHeader{.state_root = state_root}));
    EXPECT_CALL(*storage_, getEphemeralBatchAt(state_root))
        .WillOnce(testing::Invoke([&keys](auto &root) {
          auto batch =
              std::make_unique<storage::trie::EphemeralTrieBatchMock>();
          for (auto &key : keys) {
            EXPECT_CALL(*batch, tryGet(key.view()))
                .WillOnce(testing::Return(common::Buffer(root)));
          }
          return batch;
        }));

    // WHEN
    EXPECT_OUTCOME_TRUE(changes, api_->queryStorageAt(keys, at))

    // THEN
    ASSERT_EQ(changes.size(), 1);
    ASSERT_EQ(changes[0].block, at);
    ASSERT_THAT(
        changes[0].changes,
        ::testing::Each(::testing::Field(
            &StateApiImpl::StorageChangeSet::Change::key, ContainedIn(keys))));
  }

  /**
   * @given subscription id
   * @when request to unsubscribe from storage events
   * @then unsubscribe using ApiService and return if operation succeeded
   */
  TEST_F(StateApiTest, UnsubscribeStorage) {
    std::vector<uint32_t> subscription_id;
    auto expected_return = true;

    EXPECT_CALL(*api_service_, unsubscribeSessionFromIds(subscription_id))
        .WillOnce(Return(expected_return));

    api_->setApiService(api_service_);
    EXPECT_OUTCOME_SUCCESS(result, api_->unsubscribeStorage(subscription_id));
    ASSERT_EQ(expected_return, result.value());
  }

  /**
   * @when request subscription on runtime version event
   * @then subscribe on event through ApiService, return subscription id on
   * success
   */
  TEST_F(StateApiTest, SubscribeRuntimeVersion) {
    auto expected_return = 22u;

    EXPECT_CALL(*api_service_, subscribeRuntimeVersion())
        .WillOnce(Return(expected_return));

    api_->setApiService(api_service_);
    EXPECT_OUTCOME_SUCCESS(result, api_->subscribeRuntimeVersion());
    ASSERT_EQ(expected_return, result.value());
  }

  /**
   * @given subscription id
   * @when request unsubscription from runtime version event
   * @then froward request to ApiService
   */
  TEST_F(StateApiTest, UnsubscribeRuntimeVersion) {
    auto subscription_id = 42u;
    auto expected_return = StateApiImpl::Error::MAX_BLOCK_RANGE_EXCEEDED;

    EXPECT_CALL(*api_service_, unsubscribeRuntimeVersion(subscription_id))
        .WillOnce(Return(expected_return));

    api_->setApiService(api_service_);
    EXPECT_OUTCOME_ERROR(result,
                         api_->unsubscribeRuntimeVersion(subscription_id),
                         expected_return);
  }

}  // namespace kagome::api
