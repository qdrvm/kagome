/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <memory>

#include "api/service/chain/impl/chain_api_impl.hpp"
#include "api/service/chain/requests/subscribe_finalized_heads.hpp"
#include "mock/core/api/service/chain/chain_api_mock.hpp"
#include "mock/core/blockchain/block_header_repository_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/blockchain/block_storage_mock.hpp"
#include "primitives/block_header.hpp"
#include "primitives/block_data.hpp"
#include "primitives/extrinsic.hpp"
#include "primitives/block.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using kagome::api::ChainApi;
using kagome::api::ChainApiImpl;
using kagome::api::ChainApiMock;
using kagome::api::chain::request::SubscribeFinalizedHeads;
using kagome::blockchain::BlockHeaderRepositoryMock;
using kagome::blockchain::BlockTreeMock;
using kagome::blockchain::BlockStorageMock;
using kagome::common::Buffer;
using kagome::primitives::BlockHash;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockData;
using kagome::primitives::BlockBody;
using kagome::primitives::BlockId;
using kagome::primitives::BlockInfo;
using kagome::primitives::BlockNumber;
using kagome::primitives::Extrinsic;
using testing::Return;

struct ChainApiTest : public ::testing::Test {
  void SetUp() override {
    header_repo = std::make_shared<BlockHeaderRepositoryMock>();
    block_tree = std::make_shared<BlockTreeMock>();
    block_storage = std::make_shared<BlockStorageMock>();
    api = std::make_shared<ChainApiImpl>(header_repo, block_tree, block_storage);
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
  std::shared_ptr<BlockStorageMock> block_storage;

  BlockHash hash1;
  BlockHash hash2;
  BlockHash hash3;

  BlockData data{
      .hash = "4fee9b1803132954978652e4d73d4ec5b0dffae3832449cd5e4e4081d539aa22"_hash256,
      .header = BlockHeader{.parent_hash = hash1,
                                       .state_root = hash2,
                                       .extrinsics_root = hash3},
                 .body = BlockBody{
                     Extrinsic{.data = Buffer::fromHex("0011eedd33").value()},
                     Extrinsic{.data = Buffer::fromHex("55ff35").value()}},
  .receipt{},
  .message_queue{},
  .justification{}};
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

/**
 * @given chain api
 * @when get a block header by hash
 * @then the correct header will return
 */
TEST_F(ChainApiTest, GetHeader) {
  BlockId a = hash1;
  EXPECT_CALL(*header_repo, getBlockHeader(a)).WillOnce(Return(header));

  EXPECT_OUTCOME_TRUE(r, api->
                         (std::string("0x") + hash1.toHex()));
  ASSERT_EQ(r, header);
}

/**
 * @given chain api
 * @when get a block header
 * @then last block header will be returned
 */
TEST_F(ChainApiTest, GetHeaderLats) {
  BlockId a = hash1;
  EXPECT_CALL(*block_tree, getLastFinalized())
      .WillOnce(Return(BlockInfo(42, hash1)));

  EXPECT_CALL(*header_repo, getBlockHeader(a)).WillOnce(Return(header));

  EXPECT_OUTCOME_TRUE(r, api->getHeader());
  ASSERT_EQ(r, header);
}

/**
 * @given chain api
 * @when get a block by hash
 * @then the correct block data will return
 */
TEST_F(ChainApiTest, GetBlock) {
  BlockId a = hash1;
  EXPECT_CALL(*header_repo, getBlock(a)).WillOnce(Return(header));

  EXPECT_OUTCOME_TRUE(r, api->
      (std::string("0x") + hash1.toHex()));
  ASSERT_EQ(r, header);
}

/**
 * @given chain api
 * @when get a block header
 * @then last block header will be returned
 */
TEST_F(ChainApiTest, GetHeaderLats) {
  BlockId a = hash1;
  EXPECT_CALL(*block_tree, getLastFinalized())
      .WillOnce(Return(BlockInfo(42, hash1)));

  EXPECT_CALL(*header_repo, getBlockHeader(a)).WillOnce(Return(header));

  EXPECT_OUTCOME_TRUE(r, api->getHeader());
  ASSERT_EQ(r, header);
}

/**
 * @given chain api
 * @when call a subscribe new heads
 * @then the correct values are returned
 */
TEST(StateApiTest, SubscribeStorage) {
  auto chain_api = std::make_shared<ChainApiMock>();
  EXPECT_CALL(*chain_api, subscribeFinalizedHeads()).WillOnce(testing::Return(55));

  auto p = std::static_pointer_cast<ChainApi>(chain_api);
  auto sub = std::make_shared<SubscribeFinalizedHeads>(p);
  jsonrpc::Request::Parameters params;

  EXPECT_OUTCOME_SUCCESS(r, sub->init(params));
  EXPECT_OUTCOME_TRUE(result, sub->execute());
  ASSERT_EQ(result, 55);
}
