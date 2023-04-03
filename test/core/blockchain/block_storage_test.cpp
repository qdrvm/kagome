/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/block_storage_impl.hpp"

#include <gtest/gtest.h>

#include "blockchain/block_storage_error.hpp"
#include "mock/core/crypto/hasher_mock.hpp"
#include "mock/core/storage/persistent_map_mock.hpp"
#include "mock/core/storage/spaced_storage_mock.hpp"
#include "scale/scale.hpp"
#include "storage/database_error.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::blockchain::BlockStorageError;
using kagome::blockchain::BlockStorageImpl;
using kagome::common::Buffer;
using kagome::common::BufferView;
using kagome::crypto::HasherMock;
using kagome::primitives::Block;
using kagome::primitives::BlockBody;
using kagome::primitives::BlockData;
using kagome::primitives::BlockHash;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockNumber;
using kagome::storage::BufferStorageMock;
using kagome::storage::Space;
using kagome::storage::SpacedStorageMock;
using kagome::storage::trie::RootHash;
using scale::encode;
using testing::_;
using testing::Ref;
using testing::Return;

class BlockStorageTest : public testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    root_hash.fill(1);

    std::set<Space> required_spaces = {Space::kDefault,
                                       Space::kHeader,
                                       Space::kJustification,
                                       Space::kBlockBody,
                                       Space::kLookupKey};

    for (auto space : required_spaces) {
      auto storage = std::make_shared<BufferStorageMock>();
      spaces[space] = storage;
      EXPECT_CALL(*spaced_storage, getSpace(space))
          .WillRepeatedly(Return(storage));

      EXPECT_CALL(*storage, put(_, _))
          .WillRepeatedly(Return(outcome::success()));
      EXPECT_CALL(*storage, tryGetMock(_)).WillRepeatedly(Return(std::nullopt));
    }
  }
  std::shared_ptr<HasherMock> hasher = std::make_shared<HasherMock>();
  std::shared_ptr<SpacedStorageMock> spaced_storage =
      std::make_shared<SpacedStorageMock>();
  std::map<Space, std::shared_ptr<BufferStorageMock>> spaces;

  BlockHash genesis_block_hash{{'g', 'e', 'n', 'e', 's', 'i', 's'}};
  BlockHash regular_block_hash{{'r', 'e', 'g', 'u', 'l', 'a', 'r'}};
  BlockHash unhappy_block_hash{{'u', 'n', 'h', 'a', 'p', 'p', 'y'}};
  RootHash root_hash;

  std::shared_ptr<BlockStorageImpl> createWithGenesis() {
    // calculate hash of genesis block at put block header
    EXPECT_CALL(*hasher, blake2b_256(_))
        .WillRepeatedly(Return(genesis_block_hash));

    EXPECT_OUTCOME_TRUE(
        new_block_storage,
        BlockStorageImpl::create(root_hash, spaced_storage, hasher));

    return new_block_storage;
  }
};

/**
 * @given a hasher instance, a genesis block, and an empty map storage
 * @when initialising a block storage from it
 * @then initialisation will successful
 */
TEST_F(BlockStorageTest, CreateWithGenesis) {
  createWithGenesis();
}

/**
 * @given a hasher instance and an empty map storage
 * @when trying to initialise a block storage from it and storage throws an
 * error
 * @then storage will be initialized by genesis block
 */
TEST_F(BlockStorageTest, CreateWithEmptyStorage) {
  auto empty_storage = std::make_shared<BufferStorageMock>();

  // calculate hash of genesis block at put block header
  EXPECT_CALL(*hasher, blake2b_256(_))
      .WillRepeatedly(Return(genesis_block_hash));

  // check if storage contained genesis block
  EXPECT_CALL(*empty_storage, tryGetMock(_))
      .WillRepeatedly(Return(std::nullopt));

  // put genesis block into storage
  EXPECT_CALL(*empty_storage, put(_, _))
      .WillRepeatedly(Return(outcome::success()));

  ASSERT_OUTCOME_SUCCESS_TRY(
      BlockStorageImpl::create(root_hash, spaced_storage, hasher));
}

/**
 * @given a hasher instance, a genesis block, and an map storage containing the
 * block
 * @when initialising a block storage from it
 * @then initialisation will fail because the genesis block is already at the
 * underlying storage (which is actually supposed to be empty)
 */
TEST_F(BlockStorageTest, CreateWithExistingGenesis) {
  //   trying to get header of block number 0 (genesis block)
  EXPECT_CALL(*(spaces[Space::kHeader]), contains(_))
      .WillOnce(Return(outcome::success(true)));
  EXPECT_CALL(*(spaces[Space::kLookupKey]), tryGetMock(_))
      // trying to get header of block number 0 (genesis block)
      .WillOnce(Return(Buffer{genesis_block_hash}));

  ASSERT_OUTCOME_SUCCESS_TRY(
      BlockStorageImpl::create(root_hash, spaced_storage, hasher));
}

/**
 * @given a hasher instance, a genesis block, and an map storage containing the
 * block
 * @when initialising a block storage from it and storage throws an error
 * @then initialisation will fail
 */
TEST_F(BlockStorageTest, CreateWithStorageError) {
  // check if storage contained genesis block
  EXPECT_CALL(*(spaces[Space::kLookupKey]), tryGetMock(_))
      .WillOnce(Return(kagome::storage::DatabaseError::IO_ERROR));

  EXPECT_OUTCOME_ERROR(
      res,
      BlockStorageImpl::create(root_hash, spaced_storage, hasher),
      kagome::storage::DatabaseError::IO_ERROR);
}

/**
 * @given a block storage and a block that is not in storage yet
 * @when putting a block in the storage
 * @then block is successfully put
 */
TEST_F(BlockStorageTest, PutBlock) {
  auto block_storage = createWithGenesis();

  EXPECT_CALL(*hasher, blake2b_256(_)).WillOnce(Return(regular_block_hash));

  Block block;
  block.header.number = 1;
  block.header.parent_hash = genesis_block_hash;

  ASSERT_OUTCOME_SUCCESS_TRY(block_storage->putBlock(block));
}

/**
 * @given a block storage and a block that is not in storage yet
 * @when putting a block in the storage and underlying storage throws an error
 * @then block is not put and error is returned
 */
TEST_F(BlockStorageTest, PutWithStorageError) {
  auto block_storage = createWithGenesis();

  Block block;
  block.header.number = 666;
  block.header.parent_hash = genesis_block_hash;

  Buffer encoded_header{scale::encode(block.header).value()};

  EXPECT_CALL(*hasher, blake2b_256(encoded_header.view()))
      .WillOnce(Return(regular_block_hash));

  Buffer key{regular_block_hash};

  EXPECT_CALL(*(spaces[Space::kBlockBody]), put(key.view(), _))
      .WillOnce(Return(kagome::storage::DatabaseError::IO_ERROR));

  ASSERT_OUTCOME_ERROR(block_storage->putBlock(block),
                       kagome::storage::DatabaseError::IO_ERROR);
}

/**
 * @given a block storage
 * @when removing a block from it
 * @then block is successfully removed if no error occurs in the underlying
 * storage, an error is returned otherwise
 */
TEST_F(BlockStorageTest, Remove) {
  auto block_storage = createWithGenesis();

  BufferView hash(genesis_block_hash);

  Buffer encoded_header{scale::encode(BlockHeader{}).value()};

  EXPECT_CALL(*(spaces[Space::kHeader]), tryGetMock(hash))
      .WillOnce(Return(encoded_header));
  EXPECT_CALL(*(spaces[Space::kBlockBody]), remove(hash))
      .WillOnce(Return(outcome::success()));
  EXPECT_CALL(*(spaces[Space::kHeader]), remove(hash))
      .WillOnce(Return(outcome::success()));
  EXPECT_CALL(*(spaces[Space::kJustification]), remove(hash))
      .WillOnce(Return(outcome::success()));

  ASSERT_OUTCOME_SUCCESS_TRY(block_storage->removeBlock(genesis_block_hash));

  EXPECT_CALL(*(spaces[Space::kHeader]), tryGetMock(hash))
      .WillOnce(Return(std::nullopt));

  ASSERT_OUTCOME_SUCCESS_TRY(block_storage->removeBlock(genesis_block_hash));
}
