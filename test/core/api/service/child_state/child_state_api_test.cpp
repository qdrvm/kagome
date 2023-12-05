/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
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
#include "runtime/runtime_context.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using kagome::api::ChildStateApiMock;
using kagome::blockchain::BlockHeaderRepositoryMock;
using kagome::blockchain::BlockTreeMock;
using kagome::common::Buffer;
using kagome::primitives::BlockHash;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockInfo;
using kagome::primitives::BlockNumber;
using kagome::runtime::CoreMock;
using kagome::runtime::MetadataMock;
using kagome::storage::trie::PolkadotTrieCursorMock;
using kagome::storage::trie::TrieBatchMock;
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

    BlockHeader makeBlockHeaderOfStateRoot(storage::trie::RootHash state_root) {
      return BlockHeader{
          std::numeric_limits<BlockNumber>::max(),  // number
          {},                                       // parent
          state_root,                               // state root
          {},                                       // extrinsics root
          {}                                        // digest
      };
    }
  };

  /**
   * @given child_state api
   * @when get a storage value for the given key (and optionally child_state
   * root)
   * @then the correct value is returned
   */
  TEST_F(ChildStateApiTest, GetStorage) {
    EXPECT_CALL(*block_tree_, getLastFinalized())
        .WillOnce(testing::Return(BlockInfo(42, "D"_hash256)));

    EXPECT_CALL(*block_header_repo_, getBlockHeader("D"_hash256))
        .WillOnce(testing::Return(makeBlockHeaderOfStateRoot("CDE"_hash256)));
    EXPECT_CALL(*storage_, getEphemeralBatchAt("CDE"_hash256))
        .WillOnce(testing::Invoke([](auto &root) {
          auto batch = std::make_unique<TrieBatchMock>();
          static const auto key = "a"_buf;
          static const common::Buffer value{"1"_hash256};
          EXPECT_CALL(*batch, getMock(key.view()))
              .WillRepeatedly(testing::Return(value));
          return batch;
        }));
    EXPECT_CALL(*storage_, getEphemeralBatchAt("1"_hash256))
        .WillOnce(testing::Invoke([](auto &root) {
          auto batch = std::make_unique<TrieBatchMock>();
          static const auto key = "b"_buf;
          static const common::Buffer value = "2"_buf;
          EXPECT_CALL(*batch, tryGetMock(key.view()))
              .WillRepeatedly(
                  testing::Return(std::make_optional(std::cref(value))));
          return batch;
        }));

    EXPECT_OUTCOME_SUCCESS(r, api_->getStorage("a"_buf, "b"_buf, std::nullopt));
    ASSERT_EQ(r.value().value(), "2"_buf);
  }

  TEST_F(ChildStateApiTest, GetStorageAt) {
    EXPECT_CALL(*block_header_repo_, getBlockHeader("B"_hash256))
        .WillOnce(testing::Return(makeBlockHeaderOfStateRoot("ABC"_hash256)));
    EXPECT_CALL(*storage_, getEphemeralBatchAt("ABC"_hash256))
        .WillOnce(testing::Invoke([](auto &root) {
          auto batch = std::make_unique<TrieBatchMock>();
          static const auto key = "c"_buf;
          static const common::Buffer value{"3"_hash256};
          EXPECT_CALL(*batch, getMock(key.view()))
              .WillRepeatedly(testing::Return(value));
          return batch;
        }));
    EXPECT_CALL(*storage_, getEphemeralBatchAt("3"_hash256))
        .WillOnce(testing::Invoke([](auto &root) {
          auto batch = std::make_unique<TrieBatchMock>();
          static const auto key = "d"_buf;
          static const auto value = "4"_buf;
          EXPECT_CALL(*batch, tryGetMock(key.view()))
              .WillRepeatedly(
                  testing::Return(std::make_optional(std::cref(value))));
          return batch;
        }));

    EXPECT_OUTCOME_TRUE(
        r1,
        api_->getStorage(
            "c"_buf, "d"_buf, std::optional<BlockHash>{"B"_hash256}));
    ASSERT_EQ(r1.value(), "4"_buf);
  }

  /**
   * @given child storage key, key prefix, optional block hash
   * @when query keys by prefix in child storage
   * @then locate return all keys with prefix in child storage
   */
  TEST_F(ChildStateApiTest, GetKeys) {
    auto child_storage_key = "something"_buf;
    auto prefix = "ABC"_buf;
    auto prefix_opt = std::make_optional(prefix);
    auto block_hash = "12345"_hash256;
    auto block_hash_opt = std::make_optional<BlockHash>(block_hash);
    std::vector<Buffer> expected_keys{{"ABC12345"_buf}};

    EXPECT_CALL(*block_tree_, getLastFinalized())
        .WillOnce(Return(BlockInfo(10, block_hash)));
    EXPECT_CALL(*block_header_repo_, getBlockHeader(block_hash))
        .WillOnce(Return(makeBlockHeaderOfStateRoot("6789"_hash256)));
    EXPECT_CALL(*storage_, getEphemeralBatchAt("6789"_hash256))
        .WillOnce(testing::Invoke([&](auto &root) {
          auto batch = std::make_unique<TrieBatchMock>();
          EXPECT_CALL(*batch, getMock(child_storage_key.view()))
              .WillOnce(testing::Return(common::Buffer("2020"_hash256)));
          return batch;
        }));
    EXPECT_CALL(*storage_, getEphemeralBatchAt("2020"_hash256))
        .WillOnce(testing::Invoke([&](auto &root) {
          auto batch = std::make_unique<TrieBatchMock>();
          EXPECT_CALL(*batch, trieCursor())
              .WillOnce(testing::Invoke([&prefix]() {
                auto cursor = std::make_unique<PolkadotTrieCursorMock>();
                EXPECT_CALL(*cursor, seekLowerBound(prefix.view()))
                    .WillOnce(Return(outcome::success()));
                EXPECT_CALL(*cursor, isValid())
                    .WillOnce(Return(true))
                    .WillOnce(Return(false));
                EXPECT_CALL(*cursor, key())
                    .WillRepeatedly(
                        Return(std::make_optional<Buffer>("ABC12345"_buf)));
                EXPECT_CALL(*cursor, next())
                    .WillOnce(Return(outcome::success()));
                return cursor;
              }));
          return batch;
        }));

    ASSERT_EQ(
        expected_keys,
        api_->getKeys(child_storage_key, prefix_opt, block_hash_opt).value());
  }

  /**
   * @given child storage key, key prefix, page size, last key, optional block
   * hash
   * @when query keys by prefix in child storage, paginated
   * @then locate return all keys with prefix in child storage limiting output
   * down to "keys_amount"-sized pages
   */
  TEST_F(ChildStateApiTest, GetKeysPaged) {
    Buffer child_storage_key = "something"_buf;
    auto prefix = "ABC"_buf;
    auto prefix_opt = std::make_optional(prefix);
    auto keys_amount = 10u;
    auto prev_key = "prev_key"_buf;
    auto prev_key_opt = std::make_optional(prev_key);
    auto block_hash = "12345"_hash256;
    auto block_hash_opt = std::make_optional<BlockHash>(block_hash);
    std::vector<Buffer> expected_keys{{"ABC12345"_buf}};

    EXPECT_CALL(*block_tree_, getLastFinalized())
        .WillOnce(Return(BlockInfo{10, block_hash}));
    EXPECT_CALL(*block_header_repo_, getBlockHeader(block_hash))
        .WillOnce(Return(makeBlockHeaderOfStateRoot("6789"_hash256)));
    EXPECT_CALL(*storage_, getEphemeralBatchAt("6789"_hash256))
        .WillOnce(testing::Invoke([&](auto &root) {
          auto batch = std::make_unique<TrieBatchMock>();
          EXPECT_CALL(*batch, getMock(child_storage_key.view()))
              .WillOnce(testing::Return(common::Buffer("2020"_hash256)));
          return batch;
        }));
    EXPECT_CALL(*storage_, getEphemeralBatchAt("2020"_hash256))
        .WillOnce(testing::Invoke([&](auto &root) {
          auto batch = std::make_unique<TrieBatchMock>();
          EXPECT_CALL(*batch, trieCursor())
              .WillOnce(testing::Invoke([&prev_key]() {
                auto cursor = std::make_unique<PolkadotTrieCursorMock>();
                EXPECT_CALL(*cursor, seekUpperBound(prev_key.view()))
                    .WillOnce(Return(outcome::success()));
                EXPECT_CALL(*cursor, isValid())
                    .WillOnce(Return(true))
                    .WillOnce(Return(false));
                EXPECT_CALL(*cursor, key())
                    .WillRepeatedly(
                        Return(std::make_optional<Buffer>("ABC12345"_buf)));
                EXPECT_CALL(*cursor, next())
                    .WillOnce(Return(outcome::success()));
                return cursor;
              }));
          return batch;
        }));

    auto actual_keys = api_->getKeysPaged(child_storage_key,
                                          prefix_opt,
                                          keys_amount,
                                          prev_key_opt,
                                          block_hash_opt)
                           .value();

    ASSERT_EQ(expected_keys, actual_keys);
  }

  /**
   * @given child storage key, key, optional block hash
   * @when query value size in child storage
   * @then fetch value from child storage by key and get its size
   */
  TEST_F(ChildStateApiTest, GetStorageSize) {
    auto child_storage_key = "ABC"_buf;
    auto key = "DEF"_buf;
    auto block_hash = "12345"_hash256;
    auto block_hash_opt = std::make_optional<BlockHash>(block_hash);
    auto expected_result = "3030"_buf;

    EXPECT_CALL(*block_header_repo_, getBlockHeader(block_hash))
        .WillOnce(Return(makeBlockHeaderOfStateRoot("6789"_hash256)));
    auto batch = std::make_unique<TrieBatchMock>();
    EXPECT_CALL(*storage_, getEphemeralBatchAt("6789"_hash256))
        .WillOnce(testing::Invoke([&](auto &root) {
          auto batch = std::make_unique<TrieBatchMock>();
          static auto v = common::Buffer("2020"_hash256);
          EXPECT_CALL(*batch, getMock(child_storage_key.view()))
              .WillOnce(testing::Return(v));
          return batch;
        }));
    EXPECT_CALL(*storage_, getEphemeralBatchAt("2020"_hash256))
        .WillOnce(testing::Invoke([&](auto &root) {
          auto batch = std::make_unique<TrieBatchMock>();
          EXPECT_CALL(*batch, getMock(key.view()))
              .WillOnce(testing::Return(expected_result));
          return batch;
        }));

    ASSERT_OUTCOME_SUCCESS(
        size_opt, api_->getStorageSize(child_storage_key, key, block_hash_opt));
    ASSERT_TRUE(size_opt.has_value());
    ASSERT_EQ(expected_result.size(), size_opt.value());
  }

}  // namespace kagome::api
