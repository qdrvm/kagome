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

using kagome::blockchain::KeyValueBlockStorage;
using kagome::common::Buffer;
using kagome::crypto::HasherMock;
using kagome::primitives::Block;
using kagome::primitives::BlockBody;
using kagome::primitives::BlockData;
using kagome::primitives::BlockHash;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockNumber;
using kagome::scale::encode;
using kagome::storage::face::GenericStorageMock;
using testing::_;
using testing::Return;

class BlockStorageTest : public testing::Test {
 public:
  void SetUp() override {
    EXPECT_CALL(*hasher, blake2b_256(_)).WillOnce(Return(genesis_hash));
    EXPECT_CALL(*storage, get(_))
        .WillOnce(Return(
            kagome::blockchain::Error::BLOCK_NOT_FOUND))  // to check that block
                                                          // does not exist
        .WillRepeatedly(Return(
            kagome::storage::DatabaseError::
                NOT_FOUND));  // check of block data during block insertion

    root_hash.put(std::vector<uint8_t>(32ul, 1));
    EXPECT_CALL(*storage, put(_, _)).WillRepeatedly(Return(outcome::success()));
    block_storage = KeyValueBlockStorage::createWithGenesis(
                        root_hash, storage, hasher, [](auto &) {})
                        .value();
  }
  std::shared_ptr<KeyValueBlockStorage> block_storage;
  std::shared_ptr<HasherMock> hasher = std::make_shared<HasherMock>();
  std::shared_ptr<GenericStorageMock<Buffer, Buffer>> storage =
      std::make_shared<GenericStorageMock<Buffer, Buffer>>();
  Block genesis;
  BlockHash genesis_hash{{1, 2, 3, 4}};
  BlockHash block_hash{{4, 3, 2, 1}};
  Buffer root_hash;
};

/**
 * @given a hasher instance, a genesis block, and an map storage containing the
 * block
 * @when initialising a block storage from it
 * @then initialisation will fail because the genesis block is already at the
 * underlying storage (which is actually supposed to be empty)
 */
TEST_F(BlockStorageTest, CreateWithExistingGenesis) {
  EXPECT_CALL(*storage, get(_))
      .WillOnce(Return(Buffer{1, 1, 1, 1}))
      .WillOnce(Return(Buffer{1, 1, 1, 1}));
  EXPECT_OUTCOME_FALSE(res,
                       KeyValueBlockStorage::createWithGenesis(
                           root_hash, storage, hasher, [](auto &) {}));
  ASSERT_EQ(res, KeyValueBlockStorage::Error::BLOCK_EXISTS);
}

/**
 * @given a hasher instance, a genesis block, and an map storage containing the
 * block
 * @when initialising a block storage from it and storage throws an error
 * @then initialisation will fail
 */
TEST_F(BlockStorageTest, CreateWithStorageError) {
  EXPECT_CALL(*storage, get(_))
      .WillOnce(Return(Buffer{1, 1, 1, 1}))
      .WillOnce(Return(kagome::storage::DatabaseError::IO_ERROR));
  EXPECT_OUTCOME_FALSE(res,
                       KeyValueBlockStorage::createWithGenesis(
                           root_hash, storage, hasher, [](auto &) {}));
  ASSERT_EQ(res, kagome::storage::DatabaseError::IO_ERROR);
}

/**
 * @given a block storage and a block that is not in storage yet
 * @when putting a block in the storage
 * @then block is successfully put
 */
TEST_F(BlockStorageTest, PutBlock) {
  EXPECT_CALL(*hasher, blake2b_256(_)).WillOnce(Return(genesis_hash));
  EXPECT_OUTCOME_TRUE_1(block_storage->putBlock(genesis));
}

/**
 * @given a block storage and a block that is in storage already
 * @when putting a block in the storage
 * @then block is not put and an error is returned
 */
TEST_F(BlockStorageTest, PutExistingBlock) {
  EXPECT_CALL(*storage, get(_))
      .WillOnce(Return(Buffer{1, 1, 1, 1}))
      .WillOnce(Return(Buffer{1, 1, 1, 1}));
  EXPECT_OUTCOME_FALSE(res, block_storage->putBlock(genesis));
  ASSERT_EQ(res, KeyValueBlockStorage::Error::BLOCK_EXISTS);
}

/**
 * @given a block storage and a block that is not in storage yet
 * @when putting a block in the storage and underlying storage throws an error
 * @then block is not put and error is returned
 */
TEST_F(BlockStorageTest, PutWithStorageError) {
  EXPECT_CALL(*storage, get(_))
      .WillOnce(Return(Buffer{1, 1, 1, 1}))
      .WillOnce(Return(kagome::storage::DatabaseError::IO_ERROR));
  EXPECT_OUTCOME_FALSE(res, block_storage->putBlock(genesis));
  ASSERT_EQ(res, kagome::storage::DatabaseError::IO_ERROR);
}

/**
 * @given a block storage
 * @when removing a block from it
 * @then block is successfully removed if no error occurs in the underlying
 * storage, an error is returned otherwise
 */
TEST_F(BlockStorageTest, Remove) {
  EXPECT_CALL(*storage, remove(_))
      .WillOnce(Return(outcome::success()))
      .WillOnce(Return(outcome::success()));
  EXPECT_OUTCOME_TRUE_1(block_storage->removeBlock(genesis_hash, 0));

  EXPECT_CALL(*storage, remove(_))
      .WillOnce(Return(kagome::storage::DatabaseError::IO_ERROR));
  EXPECT_OUTCOME_FALSE_1(block_storage->removeBlock(genesis_hash, 0));

  EXPECT_CALL(*storage, remove(_))
      .WillOnce(Return(outcome::success()))
      .WillOnce(Return(kagome::storage::DatabaseError::IO_ERROR));
  EXPECT_OUTCOME_FALSE_1(block_storage->removeBlock(genesis_hash, 0));
}
