/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/voting_round_impl.hpp"

#include <gtest/gtest.h>
#include <boost/range/algorithm/find.hpp>
#include "clock/impl/clock_impl.hpp"
#include "consensus/grandpa/impl/chain_impl.hpp"
#include "consensus/grandpa/impl/vote_tracker_impl.hpp"
#include "consensus/grandpa/vote_graph/vote_graph_impl.hpp"
#include "core/consensus/grandpa/literals.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/blockchain/header_repository_mock.hpp"
#include "mock/core/consensus/grandpa/gossiper_mock.hpp"
#include "mock/core/consensus/grandpa/vote_crypto_provider_mock.hpp"
#include "mock/core/crypto/hasher_mock.hpp"

using namespace kagome::consensus::grandpa;
using namespace std::chrono_literals;

using kagome::blockchain::BlockTreeMock;
using kagome::blockchain::HeaderRepositoryMock;
using kagome::clock::SteadyClockImpl;
using kagome::consensus::grandpa::VoteCryptoProviderMock;
using kagome::crypto::ED25519Keypair;
using kagome::crypto::ED25519Signature;
using kagome::crypto::HasherMock;

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::Truly;

ACTION_P(onVerify, fixture) {
  if (arg0.id == fixture->kAlice) {
    return arg0.signature == fixture->kAliceSignature;
  }
  if (arg0.id == fixture->kBob) {
    return arg0.signature == fixture->kBobSignature;
  }
  if (arg0.id == fixture->kEve) {
    return arg0.signature == fixture->kEveSignature;
  }
  return false;
}

ACTION_P(onSignPrimaryPropose, fixture) {
  return fixture->preparePrimaryPropose(
      fixture->kAlice, fixture->kAliceSignature, arg0);
}
ACTION_P(onSignPrevote, fixture) {
  return fixture->preparePrevote(
      fixture->kAlice, fixture->kAliceSignature, arg0);
}
ACTION_P(onSignPrecommit, fixture) {
  return fixture->preparePrecommit(
      fixture->kAlice, fixture->kAliceSignature, arg0);
}

class VotingRoundTest : public ::testing::Test {
 public:
  void SetUp() override {
    voters_->insert(kAlice, kAliceWeight);
    voters_->insert(kBob, kBobWeight);
    voters_->insert(kEve, kEveWeight);

    keypair_.public_key = kAlice;

    // prepare lambda checking that id is known
    auto is_known_id = [this](const auto &signed_vote) {
      auto id = signed_vote.id;
      if (id == kAlice or id == kBob or id == kEve) {
        return true;
      }
      return false;
    };
    // prepare vote crypto provider
    EXPECT_CALL(*vote_crypto_provider_,
                verifyPrimaryPropose(Truly(is_known_id)))
        .WillRepeatedly(onVerify(this));
    EXPECT_CALL(*vote_crypto_provider_, verifyPrevote(Truly(is_known_id)))
        .WillRepeatedly(onVerify(this));
    EXPECT_CALL(*vote_crypto_provider_, verifyPrecommit(Truly(is_known_id)))
        .WillRepeatedly(onVerify(this));

    EXPECT_CALL(*vote_crypto_provider_, signPrimaryPropose(_))
        .WillRepeatedly(onSignPrimaryPropose(this));
    EXPECT_CALL(*vote_crypto_provider_, signPrevote(_))
        .WillRepeatedly(onSignPrevote(this));
    EXPECT_CALL(*vote_crypto_provider_, signPrecommit(_))
        .WillRepeatedly(onSignPrecommit(this));

    EXPECT_CALL(*header_repository_,
                getBlockHeader(kagome::primitives::BlockId(GENESIS_HASH)))
        .WillRepeatedly(Return(kagome::primitives::BlockHeader{.number = 1}));

    BlockInfo base{4, "C"_H};
    EXPECT_CALL(*tree_, getLastFinalized()).WillRepeatedly(Return(base));

    chain_ = std::make_shared<ChainImpl>(tree_, header_repository_);
    vote_graph_ = std::make_shared<VoteGraphImpl>(base, chain_);

    voting_round_ = std::make_shared<VotingRoundImpl>(
        voters_,
        round_number_,
        duration_,
        kAlice,
        vote_crypto_provider_,
        prevotes_,
        precommits_,
        chain_,
        vote_graph_,
        gossiper_,
        clock_,
        tree_,
        io_context_,
        [](const auto &) { std::cout << "Completed" << std::endl; });
  }

  SignedPrimaryPropose preparePrimaryPropose(
      const Id &id,
      const ED25519Signature &sig,
      const PrimaryPropose &primary_propose) {
    return SignedPrimaryPropose{
        .id = id, .signature = sig, .message = primary_propose};
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
      addBlock(new_blocks_parent_hash, block_hash, ++parent_number);

      new_blocks_parent_hash = block_hash;
    }
  }

 public:
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

  RoundNumber round_number_{0};
  Duration duration_{1000ms};
  TimePoint start_time_{42h};
  MembershipCounter counter_{0};
  std::shared_ptr<VoterSet> voters_ = std::make_shared<VoterSet>(counter_);

  ED25519Keypair keypair_;
  std::shared_ptr<VoteCryptoProviderMock> vote_crypto_provider_ =
      std::make_shared<VoteCryptoProviderMock>();

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
  std::shared_ptr<SteadyClockImpl> clock_ = std::make_shared<SteadyClockImpl>();

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

ACTION_P(onVote, test_fixture) {
  Vote vote = arg0.vote;
  kagome::visit_in_place(
      vote,
      [=](const SignedPrimaryPropose &signed_primary_propose) {
        test_fixture->voting_round_->onPrimaryPropose(signed_primary_propose);
      },
      [=](const SignedPrevote &signed_prevote) {
        // immitate receiving prevotes from other peers

        // send Alice's prevote
        test_fixture->voting_round_->onPrevote(signed_prevote);
        // send Bob's prevote
        test_fixture->voting_round_->onPrevote(
            test_fixture->preparePrevote(test_fixture->kBob,
                                         test_fixture->kBobSignature,
                                         signed_prevote.message));
        // send Eve's prevote
        test_fixture->voting_round_->onPrevote(
            test_fixture->preparePrevote(test_fixture->kEve,
                                         test_fixture->kEveSignature,
                                         signed_prevote.message));
      },
      [=](const SignedPrecommit &signed_precommit) {
        // immitate receiving precommit from other peers

        // send Alice's precommit
        test_fixture->voting_round_->onPrecommit(signed_precommit);
        // send Bob's precommit
        test_fixture->voting_round_->onPrecommit(
            test_fixture->preparePrecommit(test_fixture->kBob,
                                           test_fixture->kBobSignature,
                                           signed_precommit.message));
        // send Eve's precommit
        test_fixture->voting_round_->onPrecommit(
            test_fixture->preparePrecommit(test_fixture->kEve,
                                           test_fixture->kEveSignature,
                                           signed_precommit.message));
      });
}

ACTION_P(onFin, test_fixture) {
  test_fixture->voting_round_->onFin(arg0);
}

TEST_F(VotingRoundTest, SunnyDayScenario) {
  addBlocks(GENESIS_HASH, {"A"_H, "B"_H, "C"_H, "D"_H, "E"_H, "F"_H});
  addBlocks("E"_H, {"EA"_H, "EB"_H, "EC"_H, "ED"_H});
  addBlocks("F"_H, {"FA"_H, "FB"_H, "FC"_H});
  auto base_block_hash = "C"_H;
  auto base_block_number = 4;
  auto best_block_hash = "FC"_H;
  auto best_block_number = 10;
  EXPECT_CALL(*tree_, getBestContaining(base_block_hash, _))
      .WillRepeatedly(Return(BlockInfo{best_block_number, best_block_hash}));

  EXPECT_CALL(*tree_, getChainByBlocks(base_block_hash, best_block_hash))
      .WillRepeatedly(Return(std::vector<kagome::primitives::BlockHash>{
          "C"_H, "D"_H, "E"_H, "F"_H, "FA"_H, "FB"_H, "FC"_H}));

  RoundState last_round_state;
  last_round_state.prevote_ghost.emplace(3, "B"_H);
  last_round_state.estimate.emplace(base_block_number, base_block_hash);
  last_round_state.finalized.emplace(3, "B"_H);

  // voting round is executed by Alice. She is also Primary as round number is 0
  // and she is the first in voters set
  EXPECT_CALL(*gossiper_, vote(Truly([&](const VoteMessage &vote_message) {
    // if vote message contains primary propose with last round estimate then
    // true, false otherwise
    return kagome::visit_in_place(
        vote_message.vote,
        [&](const SignedPrimaryPropose &primary_propose) {
          return primary_propose.message.block_hash
                     == last_round_state.estimate->block_hash
                 and primary_propose.id == kAlice;
        },
        [&](const SignedPrevote &prevote) {
          std::cout << "Prevoted: " << prevote.message.block_hash.data()
                    << std::endl;
          return prevote.message.block_hash == best_block_hash
                 and prevote.id == kAlice;
        },
        [&](const SignedPrecommit &precommit) {
          std::cout << "Precommited: " << precommit.message.block_hash.data()
                    << std::endl;
          return precommit.message.block_hash == best_block_hash
                 and precommit.id == kAlice;
        });
  })))
      .WillOnce(onVote(this))   // primary propose
      .WillOnce(onVote(this))   // prevote;
      .WillOnce(onVote(this));  // precommit;

  // check that expected fin message was sent
  EXPECT_CALL(*gossiper_, fin(Truly([&](const Fin &fin_message) {
    std::cout << "Finalized: " << fin_message.vote.block_hash.data()
              << std::endl;
    auto has_all_justifications = [&](Justification just) {
      Precommit precommit{best_block_number, best_block_hash};
      SignedPrecommit alice_precommit =
          preparePrecommit(kAlice, kAliceSignature, precommit);
      SignedPrecommit bob_precommit =
          preparePrecommit(kBob, kBobSignature, precommit);
      SignedPrecommit eve_precommit =
          preparePrecommit(kEve, kEveSignature, precommit);
      bool has_alice_precommit =
          boost::find(just.items, alice_precommit) != just.items.end();
      bool has_bob_precommit =
          boost::find(just.items, bob_precommit) != just.items.end();
      bool has_eve_precommit =
          boost::find(just.items, eve_precommit) != just.items.end();

      return has_alice_precommit and has_bob_precommit and has_eve_precommit;
    };
    return fin_message.round_number == round_number_
           and fin_message.vote == BlockInfo{best_block_number, best_block_hash}
           and has_all_justifications(fin_message.justification);
  }))).WillOnce(onFin(this));

  EXPECT_CALL(*tree_, finalize(best_block_hash, _))
      .WillOnce(Return(outcome::success()));

  voting_round_->primaryPropose(last_round_state);
  voting_round_->prevote(last_round_state);
  voting_round_->precommit(last_round_state);

  io_context_->run_for(duration_ * 6);
}
