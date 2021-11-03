/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/impl/key_value_block_storage.hpp"

#include <gtest/gtest.h>
#include "blockchain/impl/common.hpp"
#include "mock/core/crypto/hasher_mock.hpp"
#include "mock/core/storage/persistent_map_mock.hpp"
#include "scale/scale.hpp"
#include "storage/database_error.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::blockchain::KeyValueBlockStorage;
using kagome::common::Buffer;
using kagome::crypto::HasherMock;
using kagome::primitives::Block;
using kagome::primitives::BlockBody;
using kagome::primitives::BlockData;
using kagome::primitives::BlockHash;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockNumber;
using   scale::encode;
using kagome::storage::face::GenericStorageMock;
using kagome::storage::trie::RootHash;
using testing::_;
using testing::Return;

class BlockStorageTest : public testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    root_hash.fill(1);
  }
  std::shared_ptr<HasherMock> hasher = std::make_shared<HasherMock>();
  std::shared_ptr<GenericStorageMock<Buffer, Buffer>> storage =
      std::make_shared<GenericStorageMock<Buffer, Buffer>>();

  BlockHash genesis_block_hash{{'g', 'e', 'n', 'e', 's', 'i', 's'}};
  BlockHash regular_block_hash{{'r', 'e', 'g', 'u', 'l', 'a', 'r'}};
  RootHash root_hash;

  KeyValueBlockStorage::BlockHandler block_handler = [](auto &) {};

  std::shared_ptr<KeyValueBlockStorage> createWithGenesis() {
    EXPECT_CALL(*hasher, blake2b_256(_))
        // calculate hash of genesis block at check existance of block
        .WillOnce(Return(genesis_block_hash))
        // calculate hash of genesis block at put block header
        .WillRepeatedly(Return(genesis_block_hash));

    EXPECT_CALL(*storage, tryGet(_))
        // trying to get last finalized block hash which not exists yet
        .WillOnce(Return(kagome::blockchain::Error::BLOCK_NOT_FOUND))
        // check of block data during block insertion
        .WillOnce(Return(std::nullopt))
        .WillOnce(Return(std::nullopt));

    EXPECT_CALL(*storage, put(_, _))
        // put key-value for lookup data
        .WillRepeatedly(Return(outcome::success()));

    EXPECT_CALL(*storage, put(_, _))
        // put key-value for lookup data
        .WillRepeatedly(Return(outcome::success()));

    EXPECT_OUTCOME_TRUE(new_block_storage,
                        KeyValueBlockStorage::createWithGenesis(
                            root_hash, storage, hasher, block_handler));

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
 * @given a hasher instance, a genesis block, and an map storage containing the
 * block
 * @when initialising a block storage from it
 * @then initialisation will fail because the genesis block is already at the
 * underlying storage (which is actually supposed to be empty)
 */
TEST_F(BlockStorageTest, CreateWithExistingGenesis) {
  EXPECT_CALL(*storage, tryGet(_))
      // trying to get last finalized block hash to ensure he not exists yet
      .WillOnce(Return(Buffer{genesis_block_hash}));

  EXPECT_OUTCOME_ERROR(
      res,
      KeyValueBlockStorage::createWithGenesis(
          root_hash, storage, hasher, block_handler),
      KeyValueBlockStorage::Error::GENESIS_BLOCK_ALREADY_EXISTS);
}

/**
 * @given a hasher instance, a genesis block, and an map storage containing the
 * block
 * @when initialising a block storage from it and storage throws an error
 * @then initialisation will fail
 */
TEST_F(BlockStorageTest, LoadFromExistingStorage) {
  EXPECT_CALL(*storage, tryGet(_))
      // trying to get last finalized block hash to ensure he not exists yet
      .WillOnce(Return(Buffer{genesis_block_hash}))
      // getting header of last finalized block
      .WillOnce(Return(Buffer{}))
      .WillOnce(Return(Buffer{ scale::encode(BlockHeader{}).value()}));

  auto new_block_storage_res =
      KeyValueBlockStorage::loadExisting(storage, hasher, block_handler);
  EXPECT_TRUE(new_block_storage_res.has_value());
}

/**
 * @given a hasher instance and an empty map storage
 * @when trying to initialise a block storage from it and storage throws an
 * error
 * @then initialisation will fail
 */
TEST_F(BlockStorageTest, LoadFromEmptyStorage) {
  auto empty_storage = std::make_shared<GenericStorageMock<Buffer, Buffer>>();

  EXPECT_CALL(*empty_storage, tryGet(_))
      // trying to get last finalized block hash to ensure he not exists yet
      .WillOnce(Return(KeyValueBlockStorage::Error::FINALIZED_BLOCK_NOT_FOUND));

  EXPECT_OUTCOME_ERROR(
      res,
      KeyValueBlockStorage::loadExisting(empty_storage, hasher, block_handler),
      KeyValueBlockStorage::Error::FINALIZED_BLOCK_NOT_FOUND);
}

/**
 * @given a hasher instance, a genesis block, and an map storage containing the
 * block
 * @when initialising a block storage from it and storage throws an error
 * @then initialisation will fail
 */
TEST_F(BlockStorageTest, CreateWithStorageError) {
  auto empty_storage = std::make_shared<GenericStorageMock<Buffer, Buffer>>();

  EXPECT_CALL(*empty_storage, tryGet(_))
      // trying to get last finalized block hash to ensure he not exists yet
      .WillOnce(Return(kagome::storage::DatabaseError::IO_ERROR));

  EXPECT_OUTCOME_ERROR(res,
                       KeyValueBlockStorage::create(
                           root_hash, empty_storage, hasher, block_handler),
                       kagome::storage::DatabaseError::IO_ERROR);
}

/**
 * @given a block storage and a block that is not in storage yet
 * @when putting a block in the storage
 * @then block is successfully put
 */
TEST_F(BlockStorageTest, PutBlock) {
  auto block_storage = createWithGenesis();

  EXPECT_CALL(*hasher, blake2b_256(_))
      .WillOnce(Return(regular_block_hash))
      .WillOnce(Return(regular_block_hash));

  EXPECT_CALL(*storage, tryGet(_))
      .WillOnce(Return(kagome::blockchain::Error::BLOCK_NOT_FOUND))
      .WillOnce(Return(kagome::blockchain::Error::BLOCK_NOT_FOUND));

  Block block;

  EXPECT_OUTCOME_TRUE_1(block_storage->putBlock(block));
}

/**
 * @given a block storage and a block that is in storage already
 * @when putting a block in the storage
 * @then block is not put and an error is returned
 */
TEST_F(BlockStorageTest, PutExistingBlock) {
  auto block_storage = createWithGenesis();

  EXPECT_CALL(*hasher, blake2b_256(_)).WillOnce(Return(genesis_block_hash));

  EXPECT_CALL(*storage, tryGet(_))
      .WillOnce(Return(Buffer{ scale::encode(BlockHeader{}).value()}))
      .WillOnce(Return(Buffer{ scale::encode(BlockBody{}).value()}));

  Block block;

  EXPECT_OUTCOME_FALSE(res, block_storage->putBlock(block));
  ASSERT_EQ(res, KeyValueBlockStorage::Error::BLOCK_EXISTS);
}

/**
 * @given a block storage and a block that is not in storage yet
 * @when putting a block in the storage and underlying storage throws an error
 * @then block is not put and error is returned
 */
TEST_F(BlockStorageTest, PutWithStorageError) {
  auto block_storage = createWithGenesis();

  EXPECT_CALL(*storage, tryGet(_))
      .WillOnce(Return(Buffer{1, 1, 1, 1}))
      .WillOnce(Return(kagome::storage::DatabaseError::IO_ERROR));

  Block block;

  EXPECT_OUTCOME_FALSE(res, block_storage->putBlock(block));
  ASSERT_EQ(res, kagome::storage::DatabaseError::IO_ERROR);
}

/**
 * @given a block storage
 * @when removing a block from it
 * @then block is successfully removed if no error occurs in the underlying
 * storage, an error is returned otherwise
 */
TEST_F(BlockStorageTest, Remove) {
  auto block_storage = createWithGenesis();

  EXPECT_CALL(*storage, remove(_))
      .WillOnce(Return(outcome::success()))
      .WillOnce(Return(outcome::success()));
  EXPECT_OUTCOME_TRUE_1(block_storage->removeBlock(genesis_block_hash, 0));

  EXPECT_CALL(*storage, remove(_))
      .WillOnce(Return(kagome::storage::DatabaseError::IO_ERROR));
  EXPECT_OUTCOME_FALSE_1(block_storage->removeBlock(genesis_block_hash, 0));

  EXPECT_CALL(*storage, remove(_))
      .WillOnce(Return(outcome::success()))
      .WillOnce(Return(kagome::storage::DatabaseError::IO_ERROR));
  EXPECT_OUTCOME_FALSE_1(block_storage->removeBlock(genesis_block_hash, 0));
}
