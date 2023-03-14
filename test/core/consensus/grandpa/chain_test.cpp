/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "consensus/grandpa/impl/environment_impl.hpp"
#include "mock/core/blockchain/block_header_repository_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/consensus/grandpa/authority_manager_mock.hpp"
#include "mock/core/network/grandpa_transmitter_mock.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::blockchain::BlockHeaderRepository;
using kagome::blockchain::BlockHeaderRepositoryMock;
using kagome::blockchain::BlockTree;
using kagome::blockchain::BlockTreeMock;
using kagome::common::Blob;
using kagome::common::Hash256;
using kagome::consensus::grandpa::AuthorityManagerMock;
using kagome::consensus::grandpa::Chain;
using kagome::consensus::grandpa::EnvironmentImpl;
using kagome::network::GrandpaTransmitterMock;
using kagome::primitives::BlockHash;
using kagome::primitives::BlockHeader;
using kagome::primitives::BlockInfo;
using kagome::primitives::BlockNumber;
using testing::_;
using testing::Return;

class ChainTest : public testing::Test {
 protected:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  /**
   * block tree with 6 blocks, contained
   * in two chains, 4 blocks each
   * h1 -> h2 -> h3 -> h4
   *          \-> h2_1 -> h2_2
   * @return
   */
  std::vector<BlockHash> mockTree() {
    std::vector<BlockHash> h{"010101"_hash256,
                             "020202"_hash256,
                             "030303"_hash256,
                             "040404"_hash256,
                             // forks from h2
                             "030101"_hash256,
                             "030202"_hash256};

    auto addBlock = [this](
                        BlockHash hash, BlockHash parent, BlockNumber number) {
      BlockHeader hh;
      hh.number = number;
      hh.parent_hash = parent;
      EXPECT_CALL(*header_repo, getBlockHeader(hash))
          .WillRepeatedly(Return(hh));
    };

    addBlock(h[3], h[2], 42);
    addBlock(h[5], h[4], 42);
    addBlock(h[4], h[2], 41);
    addBlock(h[2], h[1], 41);
    addBlock(h[1], h[0], 40);
    addBlock(h[0], BlockHash{}, 39);

    return h;
  }

  std::shared_ptr<BlockTreeMock> tree = std::make_shared<BlockTreeMock>();
  std::shared_ptr<BlockHeaderRepositoryMock> header_repo =
      std::make_shared<BlockHeaderRepositoryMock>();
  std::shared_ptr<AuthorityManagerMock> authority_manager =
      std::make_shared<AuthorityManagerMock>();
  std::shared_ptr<GrandpaTransmitterMock> grandpa_transmitter =
      std::make_shared<GrandpaTransmitterMock>();

  std::shared_ptr<Chain> chain = std::make_shared<EnvironmentImpl>(
      tree, header_repo, authority_manager, grandpa_transmitter);
};

/**
 * @given chain api instance referring to a block tree with 4 blocks in its
 * chain
 * @when obtaining the ancestry from the end of the chain to the beginning
 * @then 4 blocks of the chain are returned
 */
TEST_F(ChainTest, GetAncestry) {
  auto h1 = "010101"_hash256;
  auto h2 = "020202"_hash256;
  auto h3 = "030303"_hash256;
  auto h4 = "040404"_hash256;
  EXPECT_CALL(*tree, getChainByBlocks(h1, h4))
      .WillOnce(Return(std::vector<Hash256>{h1, h2, h3, h4}));
  ASSERT_OUTCOME_SUCCESS(blocks, chain->getAncestry(h1, h4));
  std::vector<Hash256> expected{h4, h3, h2, h1};
  ASSERT_EQ(blocks, expected);
}

/**
 * @given chain api instance referring to a block tree with 2 blocks in its
 * chain
 * @when obtaining the ancestry from the end of the chain to the beginning
 * @then 2 blocks of the chain are returned
 */
TEST_F(ChainTest, GetAncestryOfChild) {
  auto h1 = "010101"_hash256;
  auto h2 = "020202"_hash256;

  EXPECT_CALL(*tree, getChainByBlocks(h1, h2))
      .WillOnce(Return(std::vector<Hash256>{h1, h2}));
  ASSERT_OUTCOME_SUCCESS(blocks, chain->getAncestry(h1, h2));
  std::vector<Hash256> expected{h2, h1};
  ASSERT_EQ(blocks, expected);
}

/**
 * @given no special
 * @when obtaining the ancestry from h1 to itself
 * @then single block is returned, getChainByBlocks was not called
 */
TEST_F(ChainTest, GetAncestryOfItself) {
  auto h1 = "010101"_hash256;

  EXPECT_CALL(*tree, getChainByBlocks(_, _)).Times(0);
  ASSERT_OUTCOME_SUCCESS(blocks, chain->getAncestry(h1, h1));
  std::vector<Hash256> expected{h1};
  ASSERT_EQ(blocks, expected);
}

/**
 * @given chain api instance referring to a block tree with 4 blocks in its
 * chain
 * @when checking if ancestry exists from the end of the chain to the beginning
 * @then true is returned
 */
TEST_F(ChainTest, HasAncestry) {
  auto h1 = "010101"_hash256;
  auto h2 = "020202"_hash256;
  auto h3 = "030303"_hash256;

  EXPECT_CALL(*tree, hasDirectChain(h1, h2)).WillOnce(Return(true));
  ASSERT_TRUE(chain->hasAncestry(h1, h2));

  EXPECT_CALL(*tree, hasDirectChain(h3, h2)).WillOnce(Return(false));
  ASSERT_FALSE(chain->hasAncestry(h3, h2));
}

/**
 * @given no special
 * @when obtaining the ancestry from h1 to itself
 * @then true is returned, getChainByBlocks was not called
 */
TEST_F(ChainTest, HasAncestryOfItself) {
  auto h1 = "010101"_hash256;

  EXPECT_CALL(*tree, hasDirectChain(_, _)).Times(0);
  ASSERT_TRUE(chain->hasAncestry(h1, h1));
}

/**
 * @given chain api instance referring to a block tree, built in mockTree()
 * method. The best block here is h[3].
 * @when obtaining the hash of the end of the best chain containing the provided
 * block
 * @then it is successfully obtained
 */
TEST_F(ChainTest, BestChainContaining) {
  auto h = mockTree();
  EXPECT_CALL(*tree, getBestContaining(_, _))
      .WillOnce(Return(BlockInfo{42, h[3]}));
  EXPECT_CALL(*tree, getLastFinalized()).WillOnce(Return(BlockInfo{42, h[3]}));
  ASSERT_OUTCOME_SUCCESS(r, chain->bestChainContaining(h[2], std::nullopt));

  ASSERT_EQ(h[3], r.hash);
}

/**
 * @given chain api, referring to a block tree with three blocks
 * @when deterimining if a particular block is equal or descendant of another
 * block
 * @then it is true if block hashes are equal or the first one is actually the
 * ancestor of the other, and false otherwise
 */
TEST_F(ChainTest, IsEqualOrDescendantOf) {
  auto h1 = "010101"_hash256;
  auto h2 = "020202"_hash256;
  auto h3 = "030303"_hash256;
  EXPECT_CALL(*tree, hasDirectChain(h2, h2)).Times(0);
  EXPECT_CALL(*tree, hasDirectChain(h3, h1)).WillOnce(Return(false));
  EXPECT_CALL(*tree, hasDirectChain(h1, h3)).WillOnce(Return(true));

  ASSERT_TRUE(chain->isEqualOrDescendOf(h2, h2));
  ASSERT_FALSE(chain->isEqualOrDescendOf(h3, h1));
  ASSERT_TRUE(chain->isEqualOrDescendOf(h1, h3));
}
