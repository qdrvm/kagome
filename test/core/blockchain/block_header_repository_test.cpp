/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "blockchain/block_header_repository.hpp"

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <iostream>

#include "blockchain/impl/block_header_repository_impl.hpp"
#include "blockchain/impl/storage_util.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "scale/kagome_scale.hpp"
#include "scale/scale.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"
#include "testutil/scale_test_comparator.hpp"
#include "testutil/storage/base_rocksdb_test.hpp"

using kagome::blockchain::BlockHeaderRepository;
using kagome::blockchain::BlockHeaderRepositoryImpl;
using kagome::blockchain::putToSpace;
using kagome::common::Buffer;
using kagome::common::Hash256;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockInfo;
using kagome::primitives::BlockNumber;
using kagome::storage::Space;

class BlockHeaderRepository_Test : public test::BaseRocksDB_Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  BlockHeaderRepository_Test()
      : BaseRocksDB_Test(fs::path("/tmp/blockheaderrepotest.rcksdb")) {}

  void SetUp() override {
    open();

    hasher_ = std::make_shared<kagome::crypto::HasherImpl>();
    header_repo_ = std::make_shared<BlockHeaderRepositoryImpl>(rocks_, hasher_);
  }

  outcome::result<Hash256> storeHeader(BlockNumber num, BlockHeader h) {
    BlockHeader header = std::move(h);
    header.number = num;

    []() {
      ASSERT_EQ(0, kagome::scale::bitUpperBorder(0));
      ASSERT_EQ(1, kagome::scale::bitUpperBorder(1));
      ASSERT_EQ(2, kagome::scale::bitUpperBorder(3));
      ASSERT_EQ(8, kagome::scale::bitUpperBorder(0xff));
      ASSERT_EQ(6, kagome::scale::bitUpperBorder(0x3f));
    }();

    auto cmp = [](::scale::CompactInteger val) {
      size_t ref = 0;
      auto v = val;
      do {
        ++ref;
      } while ((v >>= 8) != 0);

      ASSERT_EQ(ref, kagome::scale::countBytes(val));
    };
    ::scale::CompactInteger i("1234567890123456789012345678901234567890");
    i = 1;
    cmp(i);
    cmp(::scale::CompactInteger("1234567890123456789012345678901234567890"));
    cmp(::scale::CompactInteger(0x7fff));
    cmp(::scale::CompactInteger(0xffff));
    cmp(::scale::CompactInteger(0x1ffff));
    cmp(::scale::CompactInteger(std::numeric_limits<uint64_t>::max()));
    cmp(::scale::CompactInteger(0));
    cmp(::scale::CompactInteger(1));

    [[maybe_unused]] auto __0 =
        testutil::scaleEncodeAndCompareWithRef(::scale::CompactInteger(0x3fff));

    [[maybe_unused]] auto __1 = testutil::scaleEncodeAndCompareWithRef(
        ::scale::CompactInteger(4294967295));
    OUTCOME_TRY(enc_header, testutil::scaleEncodeAndCompareWithRef(header));
    auto hash = hasher_->blake2b_256(enc_header);
    OUTCOME_TRY(putToSpace(*rocks_, Space::kHeader, hash, Buffer{enc_header}));

    auto num_to_hash_key = kagome::blockchain::blockNumberToKey(num);
    auto key_space = rocks_->getSpace(Space::kLookupKey);
    OUTCOME_TRY(key_space->put(num_to_hash_key, hash));

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
  EXPECT_OUTCOME_TRUE(enc_header,
                      testutil::scaleEncodeAndCompareWithRef(not_in_storage))
  auto hash = hasher_->blake2b_256(enc_header);
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
  EXPECT_OUTCOME_TRUE(header_by_hash, header_repo_->getBlockHeader(hash))
  auto header_should_be = getDefaultHeader();
  header_should_be.number = GetParam();
  ASSERT_EQ(header_by_hash, header_should_be);
}

TEST_P(BlockHeaderRepository_NumberParametrized_Test, bitvec) {
  auto create_bit_vec = [](size_t count) {
    ::scale::BitVec bv;
    for (size_t i = 0; i < count; ++i) {
      bv.bits.push_back((i % 2ull) == 0ull);
    }

    return bv;
  };

  for (size_t i = 0ull; i < 200ull; ++i) {
    [[maybe_unused]] auto __1 =
        testutil::scaleEncodeAndCompareWithRef(create_bit_vec(i));
  }
}

INSTANTIATE_TEST_SUITE_P(Numbers,
                         BlockHeaderRepository_NumberParametrized_Test,
                         testing::ValuesIn(ParamValues));
