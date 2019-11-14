/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/voting_round_impl.hpp"

#include <gtest/gtest.h>
#include "core/consensus/grandpa/literals.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
//#include "blockchain/impl/block_tree_impl.hpp"
#include "mock/core/blockchain/header_repository_mock.hpp"
//#include "blockchain/impl/key_value_block_header_repository.hpp"
//#include "storage/in_memory/in_memory_storage.hpp"
#include "mock/core/clock/clock_mock.hpp"
//#include "mock/core/consensus/grandpa/chain_mock.hpp"
#include "consensus/grandpa/impl/chain_impl.hpp"
#include "mock/core/consensus/grandpa/gossiper_mock.hpp"
//#include "mock/core/consensus/grandpa/vote_graph_mock.hpp"
#include "consensus/grandpa/vote_graph/vote_graph_impl.hpp"
//#include "mock/core/consensus/grandpa/vote_tracker_mock.hpp"
#include "consensus/grandpa/impl/vote_tracker_impl.hpp"
#include "mock/core/crypto/ed25519_provider_mock.hpp"
#include "mock/core/crypto/hasher_mock.hpp"

using namespace kagome::consensus::grandpa;
using namespace std::chrono_literals;

using kagome::blockchain::BlockTreeMock;
using kagome::blockchain::HeaderRepositoryMock;
// using kagome::blockchain::KeyValueBlockHeaderRepository;
// using kagome::storage::InMemoryStorage;
// using kagome::blockchain::BlockTreeImpl;
using kagome::clock::SteadyClockMock;
using kagome::crypto::ED25519Keypair;
using kagome::crypto::ED25519ProviderMock;
using kagome::crypto::ED25519Signature;
using kagome::crypto::HasherMock;

using ::testing::_;
using ::testing::Return;

class VotingRoundTest : public ::testing::Test {
 public:
  void SetUp() override {
    voters_->insert(kAlice, kAliceWeight);
    voters_->insert(kBob, kBobWeight);
    voters_->insert(kEve, kEveWeight);

    keypair_.public_key = kAlice;

    EXPECT_CALL(*header_repository_,
                getBlockHeader(kagome::primitives::BlockId(GENESIS_HASH)))
        .WillRepeatedly(Return(kagome::primitives::BlockHeader{.number = 1}));

    BlockInfo base{4, "C"_H};
    EXPECT_CALL(*tree_, getLastFinalized()).WillRepeatedly(Return(base));

    chain_ = std::make_shared<ChainImpl>(tree_, header_repository_);
    vote_graph_ = std::make_shared<VoteGraphImpl>(base, chain_);

    voting_round_ = std::make_shared<VotingRoundImpl>(voters_,
                                                      round_number_,
                                                      duration_,
                                                      keypair_,
                                                      prevotes_,
                                                      precommits_,
                                                      chain_,
                                                      vote_graph_,
                                                      gossiper_,
                                                      ed_provider_,
                                                      clock_,
                                                      tree_,
                                                      io_context_,
                                                      [](const auto &) {});
  }

  SignedPrevote preparePrevote(const Id &id,
                               const ED25519Signature &sig,
                               const Prevote &prevote) {
    return SignedPrevote{.id = id, .signature = sig, .message = prevote};
  }

  SignedPrecommit preparePrecommit(const Id &id,
                                   const ED25519Signature &sig,
                                   const Precommit &precommit) {
    return SignedPrecommit{.id = id, .signature = sig, .message = precommit};
  }

  void addBlock(BlockHash parent, BlockHash hash, BlockNumber number) {
    kagome::primitives::BlockHeader hh;
    hh.number = number;
    hh.parent_hash = parent;
    EXPECT_CALL(*header_repository_,
                getBlockHeader(kagome::primitives::BlockId(hash)))
        .WillRepeatedly(Return(hh));
  };

  void addBlocks(BlockHash parent_hash,
                 const std::vector<BlockHash> &blocks_hashes) {
    auto parent_number =
        header_repository_
            ->getBlockHeader(kagome::primitives::BlockId(parent_hash))
            .value()
            .number;

    auto new_blocks_parent_hash = parent_hash;

    for (const auto &block_hash : blocks_hashes) {
      //      kagome::primitives::BlockHeader hh;
      //      hh.number = parent_number + 1;
      //      hh.parent_hash = parent_hash;
      //      EXPECT_CALL(*header_repository_,
      //                  getBlockHeader(kagome::primitives::BlockId(block_hash)))
      //          .WillRepeatedly(Return(hh));
      //      parent_hash = block_hash;
      addBlock(new_blocks_parent_hash, block_hash, ++parent_number);

      new_blocks_parent_hash = block_hash;
    }

    //    for (int64_t block_index = blocks_hashes.size() - 1; block_index >= 0;
    //         block_index--) {
    //      for (int64_t base_block_index = block_index; base_block_index >= 0;
    //           base_block_index--) {
    //        EXPECT_CALL(*tree_,
    //                    getChainByBlocks(blocks_hashes[base_block_index],
    //                                     blocks_hashes[block_index]))
    //            .WillRepeatedly(Return(std::vector<BlockHash>(
    //                blocks_hashes.begin() + base_block_index,
    //                blocks_hashes.begin() + block_index + 1)));
    //      }
    //    }
  }

 protected:
  const BlockHash GENESIS_HASH = "genesis"_H;

  const Id kAlice = "Alice"_ID;
  const size_t kAliceWeight = 4;
  const ED25519Signature kAliceSignature = "Alice"_SIG;

  const Id kBob = "Bob"_ID;
  const size_t kBobWeight = 7;
  const ED25519Signature kBobSignature = "Bob"_SIG;

  const Id kEve = "Eve"_ID;
  const size_t kEveWeight = 3;
  const ED25519Signature kEveSignature = "Eve"_SIG;

  std::shared_ptr<VoterSet> voters_ = std::make_shared<VoterSet>();
  RoundNumber round_number_{1};
  Duration duration_{1000ms};
  TimePoint start_time_{42h};
  MembershipCounter counter_{0};

  RoundState last_round_state_;
  ED25519Keypair keypair_;

  std::shared_ptr<HasherMock> hasher_ = std::make_shared<HasherMock>();
  std::shared_ptr<HeaderRepositoryMock> header_repository_ =
      std::make_shared<HeaderRepositoryMock>();
  std::shared_ptr<BlockTreeMock> tree_ = std::make_shared<BlockTreeMock>();

  std::shared_ptr<VoteTrackerImpl<Prevote>> prevotes_ =
      std::make_shared<VoteTrackerImpl<Prevote>>();
  std::shared_ptr<VoteTrackerImpl<Precommit>> precommits_ =
      std::make_shared<VoteTrackerImpl<Precommit>>();

  std::shared_ptr<ChainImpl> chain_;
  std::shared_ptr<VoteGraphImpl> vote_graph_;
  std::shared_ptr<GossiperMock> gossiper_ = std::make_shared<GossiperMock>();
  std::shared_ptr<ED25519ProviderMock> ed_provider_ =
      std::make_shared<ED25519ProviderMock>();
  std::shared_ptr<SteadyClockMock> clock_ = std::make_shared<SteadyClockMock>();

  std::shared_ptr<boost::asio::io_context> io_context_ =
      std::make_shared<boost::asio::io_context>();

  std::shared_ptr<VotingRoundImpl> voting_round_;
};

TEST_F(VotingRoundTest, EstimateIsValid) {
  addBlocks(GENESIS_HASH, {"A"_H, "B"_H, "C"_H, "D"_H, "E"_H, "F"_H});
  addBlocks("E"_H, {"EA"_H, "EB"_H, "EC"_H, "ED"_H});
  addBlocks("F"_H, {"FA"_H, "FB"_H, "FC"_H});

  //  vote_graph_ = std::make_shared<VoteGraphImpl>(BlockInfo{4, "C"_H},
  //  chain_);

  // Alice votes
  SignedPrevote alice_vote =
      preparePrevote(kAlice, kAliceSignature, Prevote{10, "FC"_H});

  EXPECT_CALL(*tree_, getChainByBlocks("C"_H, "FC"_H))
      .WillOnce(Return(std::vector<kagome::primitives::BlockHash>{
          "C"_H, "D"_H, "E"_H, "F"_H, "FA"_H, "FB"_H, "FC"_H}));

  voting_round_->onPrevote(alice_vote);

  // Bob votes
  SignedPrevote bob_vote =
      preparePrevote(kBob, kBobSignature, Prevote{10, "ED"_H});

  EXPECT_CALL(*tree_, getChainByBlocks("C"_H, "ED"_H))
      .WillOnce(Return(std::vector<kagome::primitives::BlockHash>{
          "C"_H, "D"_H, "E"_H, "EA"_H, "EB"_H, "EC"_H, "ED"_H}));

  voting_round_->onPrevote(bob_vote);

  ASSERT_EQ(voting_round_->getCurrentState().prevote_ghost.value(),
            Prevote(6, "E"_H));
  ASSERT_EQ(voting_round_->getCurrentState().estimate.value(),
            BlockInfo(6, "E"_H));
  ASSERT_FALSE(voting_round_->completable());

  //  // Eve votes
  SignedPrevote eve_vote =
      preparePrevote(kEve, kEveSignature, Prevote{7, "F"_H});

  voting_round_->onPrevote(eve_vote);
  ASSERT_EQ(voting_round_->getCurrentState().prevote_ghost.value(),
            Prevote(6, "E"_H));
  ASSERT_EQ(voting_round_->getCurrentState().estimate.value(),
            BlockInfo(6, "E"_H));
}

TEST_F(VotingRoundTest, Finalization) {
  addBlocks(GENESIS_HASH, {"A"_H, "B"_H, "C"_H, "D"_H, "E"_H, "F"_H});
  addBlocks("E"_H, {"EA"_H, "EB"_H, "EC"_H, "ED"_H});
  addBlocks("F"_H, {"FA"_H, "FB"_H, "FC"_H});

  // Alice precommits FC
  EXPECT_CALL(*tree_, getChainByBlocks("C"_H, "FC"_H))
      .WillRepeatedly(Return(std::vector<kagome::primitives::BlockHash>{
          "C"_H, "D"_H, "E"_H, "F"_H, "FA"_H, "FB"_H, "FC"_H}));

  voting_round_->onPrecommit(
      preparePrecommit(kAlice, kAliceSignature, {10, "FC"_H}));

  // Bob precommits ED
  EXPECT_CALL(*tree_, getChainByBlocks("C"_H, "ED"_H))
      .WillRepeatedly(Return(std::vector<kagome::primitives::BlockHash>{
          "C"_H, "D"_H, "E"_H, "EA"_H, "EB"_H, "EC"_H, "ED"_H}));

  voting_round_->onPrecommit(
      preparePrecommit(kBob, kBobSignature, {10, "ED"_H}));

  ASSERT_FALSE(voting_round_->getCurrentState().finalized.has_value());

  // import some prevotes.
  // Alice Prevotes
  voting_round_->onPrevote(
      preparePrevote(kAlice, kAliceSignature, {10, "FC"_H}));

  // Bob prevotes
  voting_round_->onPrevote(preparePrevote(kBob, kBobSignature, {10, "ED"_H}));

  EXPECT_CALL(*tree_, getChainByBlocks("C"_H, "EA"_H))
      .WillRepeatedly(Return(std::vector<kagome::primitives::BlockHash>{
          "C"_H, "D"_H, "E"_H, "EA"_H}));

  // Eve prevotes
  voting_round_->onPrevote(preparePrevote(kEve, kEveSignature, {7, "EA"_H}));

  ASSERT_EQ(voting_round_->getCurrentState().finalized, BlockInfo(6, "E"_H));

  // Eve precommits
  voting_round_->onPrecommit(
      preparePrecommit(kEve, kEveSignature, {7, "EA"_H}));

  ASSERT_EQ(voting_round_->getCurrentState().finalized, BlockInfo(7, "EA"_H));
}

TEST_F(VotingRoundTest, EquivocateDoesNotDoubleCount) {
  addBlocks(GENESIS_HASH, {"A"_H, "B"_H, "C"_H, "D"_H, "E"_H, "F"_H});
  addBlocks("E"_H, {"EA"_H, "EB"_H, "EC"_H, "ED"_H});
  addBlocks("F"_H, {"FA"_H, "FB"_H, "FC"_H});
}
