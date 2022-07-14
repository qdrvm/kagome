/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/block_header_repository.hpp"

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <iostream>

#include "blockchain/impl/block_header_repository_impl.hpp"
#include "blockchain/impl/storage_util.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "scale/scale.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"
#include "testutil/storage/base_leveldb_test.hpp"

using kagome::blockchain::BlockHeaderRepository;
using kagome::blockchain::BlockHeaderRepositoryImpl;
using kagome::blockchain::numberAndHashToLookupKey;
using kagome::blockchain::numberToIndexKey;
using kagome::blockchain::prependPrefix;
using kagome::blockchain::putWithPrefix;
using kagome::blockchain::prefix::Prefix;
using kagome::common::Buffer;
using kagome::common::Hash256;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockInfo;
using kagome::primitives::BlockNumber;

class BlockHeaderRepository_Test : public test::BaseLevelDB_Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  BlockHeaderRepository_Test()
      : BaseLevelDB_Test(fs::path("/tmp/blockheaderrepotest.lvldb")) {}

  void SetUp() override {
    open();

    hasher_ = std::make_shared<kagome::crypto::HasherImpl>();
    header_repo_ = std::make_shared<BlockHeaderRepositoryImpl>(db_, hasher_);
  }

  outcome::result<Hash256> storeHeader(BlockNumber num, BlockHeader h) {
    BlockHeader header = std::move(h);
    header.number = num;
    OUTCOME_TRY(enc_header, scale::encode(header));
    auto hash = hasher_->blake2b_256(enc_header);
    OUTCOME_TRY(putWithPrefix(
        *db_, Prefix::HEADER, header.number, hash, Buffer{enc_header}));
    OUTCOME_TRY(kagome::blockchain::putNumberToIndexKey(*db_, {num, hash}));

    return hash;
  }

  BlockHeader getDefaultHeader() {
    BlockHeader h{};
    h.number = 42;
    h.extrinsics_root = "DEADBEEF"_hash256;
    h.parent_hash = "ABCDEF"_hash256;
    h.state_root = "010203"_hash256;
    return h;
  }

  std::shared_ptr<kagome::crypto::Hasher> hasher_;
  std::shared_ptr<BlockHeaderRepository> header_repo_;
};

class BlockHeaderRepository_NumberParametrized_Test
    : public BlockHeaderRepository_Test,
      public testing::WithParamInterface<BlockNumber> {};

const std::vector<BlockNumber> ParamValues = {1, 42, 12345, 0, 0xFFFFFFFF};

/**
 * @given HeaderBackend instance with several headers in the storage
 * @when accessing a header that wasn't put into storage
 * @then result is error
 */
TEST_F(BlockHeaderRepository_Test, UnexistingHeader) {
  auto chosen_number = ParamValues[0];
  for (auto &c : ParamValues) {
    if (c != chosen_number) {
      EXPECT_OUTCOME_TRUE_1(storeHeader(c, getDefaultHeader()))
    }
  }
  BlockHeader not_in_storage = getDefaultHeader();
  not_in_storage.number = chosen_number;
  EXPECT_OUTCOME_TRUE(enc_header, scale::encode(not_in_storage))
  auto hash = hasher_->blake2b_256(enc_header);
  EXPECT_OUTCOME_FALSE_1(header_repo_->getBlockHeader(chosen_number))
  EXPECT_OUTCOME_FALSE_1(header_repo_->getBlockHeader(hash))
  EXPECT_OUTCOME_FALSE_1(header_repo_->getHashById(chosen_number))
  EXPECT_OUTCOME_FALSE_1(header_repo_->getNumberById(hash))

  // doesn't require access to storage, as it basically returns its argument
  EXPECT_OUTCOME_TRUE_1(header_repo_->getHashById(hash))
  EXPECT_OUTCOME_TRUE_1(header_repo_->getNumberById(chosen_number))
}

/**
 * @given HeaderBackend instance
 * @when learning block hash by its number through HeaderBackend
 * @then resulting hash is equal to the original hash of the block for both
 * retrieval through getHashByNumber and getHashById
 */
TEST_P(BlockHeaderRepository_NumberParametrized_Test, GetHashByNumber) {
  EXPECT_OUTCOME_TRUE(hash, storeHeader(GetParam(), getDefaultHeader()))
  EXPECT_OUTCOME_TRUE(maybe_hash, header_repo_->getHashByNumber(GetParam()))
  ASSERT_THAT(hash, testing::ContainerEq(maybe_hash));
  EXPECT_OUTCOME_TRUE(maybe_another_hash, header_repo_->getHashById(GetParam()))
  ASSERT_THAT(hash, testing::ContainerEq(maybe_another_hash));
}

/**
 * @given HeaderBackend instance
 * @when learning block number by its hash through HeaderBackend
 * @then resulting number is equal to the original block number for both
 * retrieval through getNumberByHash and getNumberById
 */
TEST_P(BlockHeaderRepository_NumberParametrized_Test, GetNumberByHash) {
  EXPECT_OUTCOME_TRUE(hash, storeHeader(GetParam(), getDefaultHeader()))
  EXPECT_OUTCOME_TRUE(maybe_number, header_repo_->getNumberByHash(hash))
  ASSERT_EQ(GetParam(), maybe_number);
  EXPECT_OUTCOME_TRUE(maybe_another_number,
                      header_repo_->getNumberById(GetParam()))
  ASSERT_EQ(GetParam(), maybe_another_number);
}

/**
 * @given HeaderBackend instance
 * @when retrieving a block header by its id
 * @then the same header that was put into the storage is returned, regardless
 * of whether the id contained a number or a hash
 */
TEST_P(BlockHeaderRepository_NumberParametrized_Test, GetHeader) {
  EXPECT_OUTCOME_TRUE(hash, storeHeader(GetParam(), getDefaultHeader()))
  EXPECT_OUTCOME_TRUE(header_by_num, header_repo_->getBlockHeader(GetParam()))
  EXPECT_OUTCOME_TRUE(header_by_hash, header_repo_->getBlockHeader(hash))
  auto header_should_be = getDefaultHeader();
  header_should_be.number = GetParam();
  ASSERT_EQ(header_by_hash, header_should_be);
  ASSERT_EQ(header_by_num, header_should_be);
}

INSTANTIATE_TEST_SUITE_P(Numbers,
                         BlockHeaderRepository_NumberParametrized_Test,
                         testing::ValuesIn(ParamValues));
