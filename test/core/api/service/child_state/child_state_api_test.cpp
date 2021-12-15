/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "api/service/child_state/impl/child_state_api_impl.hpp"
#include "core/storage/trie/polkadot_trie_cursor_dummy.hpp"
#include "mock/core/api/service/child_state/child_state_api_mock.hpp"
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

using kagome::api::ChildStateApiMock;
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

  class ChildStateApiTest : public ::testing::Test {
   public:
    void SetUp() override {
      api_ = std::make_unique<api::ChildStateApiImpl>(
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

    std::unique_ptr<api::ChildStateApiImpl> api_{};
  };

  /**
   * @given child_state api
   * @when get a storage value for the given key (and optionally child_state root)
   * @then the correct value is returned
   */
  TEST_F(ChildStateApiTest, GetStorage) {
    EXPECT_CALL(*block_tree_, getLastFinalized())
        .WillOnce(testing::Return(BlockInfo(42, "D"_hash256)));
    primitives::BlockId did = "D"_hash256;
    EXPECT_CALL(*block_header_repo_, getBlockHeader(did))
        .WillOnce(testing::Return(BlockHeader{.state_root = "CDE"_hash256}));
    EXPECT_CALL(*storage_, getEphemeralBatchAt(_))
        .WillRepeatedly(testing::Invoke([](auto &root) {
          auto batch = std::make_unique<EphemeralTrieBatchMock>();
          EXPECT_CALL(*batch, tryGet("a"_buf))
              .WillRepeatedly(testing::Return("1"_buf));
          return batch;
        }));

    EXPECT_OUTCOME_TRUE(r, api_->getStorage("a"_buf, "b"_buf, std::nullopt));
    ASSERT_EQ(r.value(), "1"_buf);

    primitives::BlockId bid = "B"_hash256;
    EXPECT_CALL(*block_header_repo_, getBlockHeader(bid))
        .WillOnce(testing::Return(BlockHeader{.state_root = "ABC"_hash256}));

    EXPECT_OUTCOME_TRUE(r1, api_->getStorage("a"_buf, "B"_buf, std::optional<BlockHash>{"111213"_hash256}));
    ASSERT_EQ(r1.value(), "1"_buf);
  }

  class GetKeysPagedTest : public ::testing::Test {
   public:
    void SetUp() override {
      auto storage = std::make_shared<TrieStorageMock>();
      block_header_repo_ = std::make_shared<BlockHeaderRepositoryMock>();
      block_tree_ = std::make_shared<BlockTreeMock>();
      auto runtime_core = std::make_shared<CoreMock>();
      auto metadata = std::make_shared<MetadataMock>();

      api_ = std::make_shared<api::ChildStateApiImpl>(
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
    std::shared_ptr<api::ChildStateApiImpl> api_;

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
   * @given child_state api with cursor over predefined set of key-vals
   * @when getKeysPaged invoked with no prefix
   * @then expected amount of keys from beginning of cursor are returned
   */
  TEST_F(GetKeysPagedTest, EmptyParamsTest) {
    EXPECT_OUTCOME_TRUE(
        val, api_->getKeysPaged(common::Buffer{}, std::nullopt, 2, std::nullopt, std::nullopt));
    ASSERT_THAT(val, ElementsAre("0102"_hex2buf, "0103"_hex2buf));
  }

  /**
   * @given child_state api with cursor over predefined set of key-vals
   * @when getKeysPaged invoked with prefix
   * @then expected amount of keys with provided prefix are returned
   */
  TEST_F(GetKeysPagedTest, NonEmptyPrefixTest) {
    EXPECT_OUTCOME_TRUE(
        val, api_->getKeysPaged("0607"_hex2buf, "0607"_hex2buf, 3, std::nullopt, std::nullopt));
    ASSERT_THAT(
        val, ElementsAre("0607"_hex2buf, "060708"_hex2buf, "06070801"_hex2buf));
  }

  /**
   * @given child_state api with cursor over predefined set of key-vals
   * @when getKeysPaged invoked with prefix and prev_key
   * @then exepected amount of keys after provided prev_key are returned
   */
  TEST_F(GetKeysPagedTest, NonEmptyPrevKeyTest) {
    EXPECT_OUTCOME_TRUE(
        val, api_->getKeysPaged("0607"_hex2buf, "06"_hex2buf, 3, "0607"_hex2buf, std::nullopt));
    ASSERT_THAT(
        val,
        ElementsAre("060708"_hex2buf, "06070801"_hex2buf, "06070802"_hex2buf));
  }

  /**
   * @given child_state api with cursor over predefined set of key-vals
   * @when getKeysPaged invoked with non-empty prev_key and non-empty prefix
   * that is bigger than prev_key
   * @then expected amount of keys with provided prefix after prev_key are
   * returned
   */
  TEST_F(GetKeysPagedTest, PrefixBiggerThanPrevkey) {
    EXPECT_OUTCOME_TRUE(
        val,
        api_->getKeysPaged("0607"_hex2buf, "060708"_hex2buf, 5, "06"_hex2buf, std::nullopt));
    ASSERT_THAT(val,
                ElementsAre("060708"_hex2buf,
                            "06070801"_hex2buf,
                            "06070802"_hex2buf,
                            "06070803"_hex2buf));
  }
}  // namespace kagome::api
