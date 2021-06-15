/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "api/service/state/impl/state_api_impl.hpp"
#include "api/service/state/requests/get_metadata.hpp"
#include "api/service/state/requests/subscribe_storage.hpp"
#include "core/storage/trie/polkadot_trie_cursor_dummy.hpp"
#include "mock/core/api/service/state/state_api_mock.hpp"
#include "mock/core/blockchain/block_header_repository_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/runtime/core_mock.hpp"
#include "mock/core/runtime/metadata_mock.hpp"
#include "mock/core/storage/trie/polkadot_trie_cursor_mock.h"
#include "mock/core/storage/trie/trie_batches_mock.hpp"
#include "mock/core/storage/trie/trie_storage_mock.hpp"
#include "primitives/block_header.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

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
    auto metadata = std::make_shared<MetadataMock>();

    api::StateApiImpl api{
        block_header_repo, storage, block_tree, runtime_core, metadata};

    EXPECT_CALL(*block_tree, getLastFinalized())
        .WillOnce(testing::Return(BlockInfo(42, "D"_hash256)));
    primitives::BlockId did = "D"_hash256;
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

    primitives::BlockId bid = "B"_hash256;
    EXPECT_CALL(*block_header_repo, getBlockHeader(bid))
        .WillOnce(testing::Return(BlockHeader{.state_root = "ABC"_hash256}));

    EXPECT_OUTCOME_TRUE(r1, api.getStorage("a"_buf, "B"_hash256));
    ASSERT_EQ(r1, "1"_buf);
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
            EXPECT_CALL(*batch, trieCursorProxy())
                .WillRepeatedly(
                    Return(new storage::trie::PolkadotTrieCursorDummy(
                        lex_sorted_vals)));
            return batch;
          }));
    }

   protected:
    std::shared_ptr<BlockHeaderRepositoryMock> block_header_repo_;
    std::shared_ptr<BlockTreeMock> block_tree_;
    std::shared_ptr<api::StateApiImpl> api_;

    const std::map<Buffer, Buffer> lex_sorted_vals{
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
        val, api_->getKeysPaged(boost::none, 2, boost::none, boost::none));
    ASSERT_THAT(val, ElementsAre("0102"_hex2buf, "0103"_hex2buf));
  }

  /**
   * @given state api with cursor over predefined set of key-vals
   * @when getKeysPaged invoked with prefix
   * @then expected amount of keys with provided prefix are returned
   */
  TEST_F(GetKeysPagedTest, NonEmptyPrefixTest) {
    EXPECT_OUTCOME_TRUE(
        val, api_->getKeysPaged("0607"_hex2buf, 3, boost::none, boost::none));
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
        val, api_->getKeysPaged("06"_hex2buf, 3, "0607"_hex2buf, boost::none));
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
        api_->getKeysPaged("060708"_hex2buf, 5, "06"_hex2buf, boost::none));
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
  TEST(StateApiTest, GetRuntimeVersion) {
    auto storage = std::make_shared<TrieStorageMock>();
    auto block_header_repo = std::make_shared<BlockHeaderRepositoryMock>();
    auto block_tree = std::make_shared<BlockTreeMock>();
    auto runtime_core = std::make_shared<CoreMock>();
    auto metadata = std::make_shared<MetadataMock>();

    api::StateApiImpl api{
        block_header_repo, storage, block_tree, runtime_core, metadata};

    primitives::Version test_version{.spec_name = "dummy_sn",
                                     .impl_name = "dummy_in",
                                     .authoring_version = 0x101,
                                     .spec_version = 0x111,
                                     .impl_version = 0x202};

    EXPECT_CALL(*runtime_core, version())
        .WillOnce(testing::Return(test_version));

    {
      EXPECT_OUTCOME_TRUE(result, api.getRuntimeVersion(boost::none));
      ASSERT_EQ(result, test_version);
    }

    primitives::BlockHash hash = "T"_hash256;
    EXPECT_CALL(*runtime_core, versionAt(hash))
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
  TEST(StateApiTest, SubscribeStorageInvalidData) {
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
  TEST(StateApiTest, SubscribeStorageWithoutPrefix) {
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
  TEST(StateApiTest, SubscribeStorageBadBoy) {
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
  TEST(StateApiTest, GetMetadata) {
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
}  // namespace kagome::api
