/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "api/service/chain/impl/chain_api_impl.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/blockchain/block_header_repository_mock.hpp"
#include "primitives/block_header.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using kagome::api::ChainApi;
using kagome::api::ChainApiImpl;
using kagome::blockchain::BlockTreeMock;
using kagome::blockchain::BlockHeaderRepositoryMock;
using kagome::common::Buffer;
using kagome::primitives::BlockHash;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockInfo;
using kagome::primitives::BlockNumber;
using testing::Return;

struct ChainApiTest : public ::testing::Test {
  void SetUp() override {
    header_repo = std::make_shared<BlockHeaderRepositoryMock>();
    block_tree = std::make_shared<BlockTreeMock>();
    api = std::make_shared<ChainApiImpl>(header_repo, block_tree);
    hash1 =
        "4fee9b1803132954978652e4d73d4ec5b0dffae3832449cd5e4e4081d539aa22"_hash256;
    hash2 =
        "46781d9a3350a0e02dbea4b5e7aee7c139331a65b2cd736bb45a824c2f3ffd1a"_hash256;
    hash3 =
        "0f82403bcd4f7d4d23ce04775d112cd5dede13633924de6cb048d2676e322950"_hash256;
  }

  std::shared_ptr<BlockHeaderRepositoryMock> header_repo;
  std::shared_ptr<BlockTreeMock> block_tree;
  std::shared_ptr<ChainApi> api;

  BlockHash hash1;
  BlockHash hash2;
  BlockHash hash3;
};

/**
 * @given chain api
 * @when get a block hash value without parameter
 * @then last finalized block hash value is returned
 */
TEST_F(ChainApiTest, GetBlockHashNoParam) {
  EXPECT_CALL(*block_tree, getLastFinalized())
      .WillOnce(Return(BlockInfo(42, "D"_hash256)));

  EXPECT_OUTCOME_TRUE(r, api->getBlockHash());
  ASSERT_EQ(r, "D"_hash256);
}

/**
 * @given chain api
 * @when get a block hash value for the given block number
 * @then the correct hash value is returned
 */
TEST_F(ChainApiTest, GetBlockHashByNumber) {
  //  kagome::primitives::BlockId did = "D"_hash256;
  EXPECT_CALL(*header_repo, getHashByNumber(42))
      .WillOnce(Return("CDE"_hash256));

  EXPECT_OUTCOME_TRUE(r, api->getBlockHash(42));
  ASSERT_EQ(r, "CDE"_hash256);
}

/**
 * @given chain api
 * @when get a block hash value for the given hex-encoded block number
 * @then the correct hash value is returned
 */
TEST_F(ChainApiTest, GetBlockHashByHexNumber) {
  EXPECT_CALL(*header_repo, getHashByNumber(42))
      .WillOnce(Return("CDE"_hash256));

  EXPECT_OUTCOME_TRUE(r, api->getBlockHash("0x2a"));
  ASSERT_EQ(r, "CDE"_hash256);
}

/**
 * @given chain api and 3 predefined block hashes
 * @when call getBlockHash method for the given predefined array
 * @then the correct vector of hash values is returned
 */
TEST_F(ChainApiTest, GetBlockHashArray) {
  EXPECT_CALL(*header_repo, getHashByNumber(50)).WillOnce(Return(hash1));
  EXPECT_CALL(*header_repo, getHashByNumber(100)).WillOnce(Return(hash2));
  EXPECT_CALL(*header_repo, getHashByNumber(200)).WillOnce(Return(hash3));
  std::vector<boost::variant<uint32_t, std::string>> request_data = {
      50, "0x64", 200};
  EXPECT_OUTCOME_TRUE(
      r,
      api->getBlockHash(std::vector<boost::variant<BlockNumber, std::string>>(
          {50, "0x64", 200})));
  ASSERT_EQ(r, std::vector<BlockHash>({hash1, hash2, hash3}));
}
