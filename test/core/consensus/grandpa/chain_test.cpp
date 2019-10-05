/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include "consensus/grandpa/impl/chain_impl.cpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/blockchain/header_repository_mock.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using kagome::blockchain::BlockHeaderRepository;
using kagome::blockchain::BlockTree;
using kagome::blockchain::BlockTreeMock;
using kagome::blockchain::HeaderRepositoryMock;
using kagome::common::Blob;
using kagome::common::Hash256;
using kagome::consensus::grandpa::Chain;
using kagome::consensus::grandpa::ChainImpl;
using kagome::primitives::BlockHash;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockNumber;
using testing::_;
using testing::Return;

class ChainTest : public testing::Test {
 public:
  void SetUp() override {}

  std::shared_ptr<BlockTreeMock> tree = std::make_shared<BlockTreeMock>();
  std::shared_ptr<HeaderRepositoryMock> header_repo =
      std::make_shared<HeaderRepositoryMock>();

  std::shared_ptr<Chain> chain = std::make_shared<ChainImpl>(tree, header_repo);
};

/**
 * @given chain api instance referring to a block tree with 4 blocks in its chain
 * @when obtaining the ancestry from the end of the chain to the beginning
 * @then 2 blocks in between the head and the tail of the chain are returned
 */
TEST_F(ChainTest, Ancestry) {
  auto h1 = "010101"_hash256;
  auto h2 = "020202"_hash256;
  auto h3 = "030303"_hash256;
  auto h4 = "040404"_hash256;
  EXPECT_CALL(*tree, getChainByBlocks(_, _))
      .WillOnce(Return(std::vector<Hash256>{h1, h2, h3, h4}));
  EXPECT_OUTCOME_TRUE(blocks, chain->getAncestry(h1, h4));
  std::vector<Hash256> expected{h3, h2};
  ASSERT_EQ(blocks, expected);
}

/**
 * @given chain api instance referring to a block tree with 4 blocks in its chain
 * @when obtaining the hash of the end of the best chain containing the provided block
 * @then it is successfully obtained
 */
TEST_F(ChainTest, BestChainContaining) {
  auto h1 = "010101"_hash256;
  auto h2 = "020202"_hash256;
  auto h3 = "030303"_hash256;
  auto h4 = "040404"_hash256;

  EXPECT_CALL(*tree, getBestContaining(_, _))
      .WillOnce(Return(BlockTree::BlockInfo{42, h4}));

  auto addBlock = [this](BlockHash hash, BlockHash parent, BlockNumber number) {
    BlockHeader hh;
    hh.number = number;
    hh.parent_hash = parent;
    EXPECT_CALL(*header_repo, getBlockHeader(kagome::primitives::BlockId(hash))).WillRepeatedly(Return(hh));
  };

  addBlock(h4, h3, 42);
  addBlock(h3, h2, 41);
  addBlock(h2, h1, 40);
  addBlock(h1, BlockHash{}, 39);

  EXPECT_OUTCOME_TRUE(r, chain->bestChainContaining(h3));

  ASSERT_EQ(h4, r.hash);
}
