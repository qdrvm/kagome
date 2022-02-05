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
   * @when get a storage value for the given key (and optionally child_state
   * root)
   * @then the correct value is returned
   */
  TEST_F(ChildStateApiTest, GetStorage) {
    EXPECT_CALL(*block_tree_, getLastFinalized())
        .WillOnce(testing::Return(BlockInfo(42, "D"_hash256)));
    primitives::BlockId did = "D"_hash256;
    EXPECT_CALL(*block_header_repo_, getBlockHeader(did))
        .WillOnce(testing::Return(BlockHeader{.state_root = "CDE"_hash256}));
    EXPECT_CALL(*storage_, getEphemeralBatchAt("CDE"_hash256))
        .WillOnce(testing::Invoke([](auto &root) {
          auto batch = std::make_unique<EphemeralTrieBatchMock>();
          static const auto key = "a"_buf;
          static const common::Buffer value {"1"_hash256};
          EXPECT_CALL(*batch, get(key.view()))
              .WillRepeatedly(testing::Return(value));
          return batch;
        }));
    EXPECT_CALL(*storage_, getEphemeralBatchAt("1"_hash256))
        .WillOnce(testing::Invoke([](auto &root) {
          auto batch = std::make_unique<EphemeralTrieBatchMock>();
          static const auto key = "b"_buf;
          static const common::Buffer value = "2"_buf;
          auto value_ref_opt = std::make_optional(std::cref(value));
          EXPECT_CALL(*batch, tryGet(key.view()))
              .WillRepeatedly(testing::Return(value_ref_opt));
          return batch;
        }));

    EXPECT_OUTCOME_SUCCESS(r, api_->getStorage("a"_buf, "b"_buf, std::nullopt));
    auto rv = r.value().value();
    auto v = rv.get();
    ASSERT_EQ(v, "2"_buf);
  }

  TEST_F(ChildStateApiTest, GetStorageAt) {
    primitives::BlockId bid = "B"_hash256;
    EXPECT_CALL(*block_header_repo_, getBlockHeader(bid))
        .WillOnce(testing::Return(BlockHeader{.state_root = "ABC"_hash256}));
    EXPECT_CALL(*storage_, getEphemeralBatchAt("ABC"_hash256))
        .WillOnce(testing::Invoke([](auto &root) {
          auto batch = std::make_unique<EphemeralTrieBatchMock>();
          static const auto key = "c"_buf;
          EXPECT_CALL(*batch, get(key.view()))
              .WillRepeatedly(testing::Return(common::Buffer("3"_hash256)));
          return batch;
        }));
    EXPECT_CALL(*storage_, getEphemeralBatchAt("3"_hash256))
        .WillOnce(testing::Invoke([](auto &root) {
          auto batch = std::make_unique<EphemeralTrieBatchMock>();
          static const auto key = "d"_buf;
          EXPECT_CALL(*batch, tryGet(key.view()))
              .WillRepeatedly(testing::Return("4"_buf));
          return batch;
        }));

    EXPECT_OUTCOME_TRUE(
        r1,
        api_->getStorage(
            "c"_buf, "d"_buf, std::optional<BlockHash>{"B"_hash256}));
    ASSERT_EQ(r1.value().get(), "4"_buf);
  }
}  // namespace kagome::api
