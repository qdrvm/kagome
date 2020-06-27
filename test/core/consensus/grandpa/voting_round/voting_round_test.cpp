/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <boost/range/algorithm/find.hpp>

#include "clock/impl/clock_impl.hpp"
#include "consensus/grandpa/impl/environment_impl.hpp"
#include "consensus/grandpa/impl/vote_tracker_impl.hpp"
#include "consensus/grandpa/impl/voting_round_error.hpp"
#include "consensus/grandpa/impl/voting_round_impl.hpp"
#include "consensus/grandpa/vote_graph/vote_graph_impl.hpp"
#include "core/consensus/grandpa/literals.hpp"
#include "mock/core/blockchain/block_header_repository_mock.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/consensus/grandpa/environment_mock.hpp"
#include "mock/core/consensus/grandpa/gossiper_mock.hpp"
#include "mock/core/consensus/grandpa/vote_crypto_provider_mock.hpp"
#include "mock/core/crypto/hasher_mock.hpp"

using namespace kagome::consensus::grandpa;
using namespace std::chrono_literals;

using kagome::blockchain::BlockTreeMock;
using kagome::clock::SteadyClockImpl;
using kagome::consensus::grandpa::EnvironmentMock;
using kagome::consensus::grandpa::GrandpaConfig;
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

    // lambda checking that id is matches to one of the known ones
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

    BlockInfo base{4, "C"_H};

    env_ = std::make_shared<EnvironmentMock>();
    vote_graph_ = std::make_shared<VoteGraphImpl>(base, env_);

    GrandpaConfig config{.voters = voters_,
                         .round_number = round_number_,
                         .duration = duration_,
                         .peer_id = kAlice};

    voting_round_ = std::make_shared<VotingRoundImpl>(config,
                                                      env_,
                                                      vote_crypto_provider_,
                                                      prevotes_,
                                                      precommits_,
                                                      vote_graph_,
                                                      clock_,
                                                      io_context_);
  }

  SignedMessage preparePrimaryPropose(const Id &id,
                                      const ED25519Signature &sig,
                                      const PrimaryPropose &primary_propose) {
    return SignedMessage{
        .message = primary_propose, .signature = sig, .id = id};
  }

  SignedMessage preparePrevote(const Id &id,
                               const ED25519Signature &sig,
                               const Prevote &prevote) {
    return SignedMessage{.message = prevote, .signature = sig, .id = id};
  }

  SignedMessage preparePrecommit(const Id &id,
                                 const ED25519Signature &sig,
                                 const Precommit &precommit) {
    return SignedMessage{.message = precommit, .signature = sig, .id = id};
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

  std::shared_ptr<VoteTrackerImpl> prevotes_ =
      std::make_shared<VoteTrackerImpl>();
  std::shared_ptr<VoteTrackerImpl> precommits_ =
      std::make_shared<VoteTrackerImpl>();

  std::shared_ptr<EnvironmentMock> env_;
  std::shared_ptr<VoteGraphImpl> vote_graph_;
  std::shared_ptr<Clock> clock_ = std::make_shared<SteadyClockImpl>();

  std::shared_ptr<boost::asio::io_context> io_context_ =
      std::make_shared<boost::asio::io_context>();

  std::shared_ptr<VotingRoundImpl> voting_round_;
};

/**
 * Note: _h is postfix to generate hash
 * We assume that hashes are sorted in alphabetical order (i.e. block with hash
 * "B"_h is higher by one block than "A"_h). Forks might have second letter:
 * {"FA"_h, "FB"_h} is fork starting from "F"_h
 *
 * Prevote{N, H}, Precommit{N, H}, BlockInfo{N, H} mean prevote or precommit for
 * the block with number N and hash H
 */

/**
 * Check that prevote ghost and estimates are updated correctly
 *
 * @given Network of:
 * Alice with weight 4,
 * Bob with weight 7 and
 * Eve with weight 3
 * @and vote graph with base in Block with number 4 and hash "C"_h
 * @when
 * 1. Alice prevotes Prevote{10, "FC"_h}
 * 2. Bob prevotes Prevote{10, "ED"_h}
 * 3. Eve prevotes Prevote{6 , "F"_h}
 * @then
 * 1. After Bob prevoted, prevote_ghost will be Prevote{6, "E"_h} (as this is
 * the best ancestor of "FC"_h and "ED"_h with supermajority)
 * 2. After Bob prevoted, estimate will be BlockInfo{6, "E"_h} (as this is the
 * best block that can be finalized)
 * 3. After Eve prevotes, prevote_ghost and estimate should not change (as her
 * weight is not enough to have supermajority for "F"_h)
 */
TEST_F(VotingRoundTest, EstimateIsValid) {
  // given (in fixture)

  // when 1.
  // Alice prevotes
  auto alice_vote =
      preparePrevote(kAlice, kAliceSignature, Prevote{10, "FC"_H});

  EXPECT_CALL(*env_, getAncestry("C"_H, "FC"_H))
      .WillOnce(Return(std::vector<kagome::primitives::BlockHash>{
          "FB"_H, "FA"_H, "F"_H, "E"_H, "D"_H}));

  voting_round_->onPrevote(alice_vote);

  // when 2.
  // Bob prevotes
  auto bob_vote = preparePrevote(kBob, kBobSignature, Prevote{10, "ED"_H});

  EXPECT_CALL(*env_, getAncestry("C"_H, "ED"_H))
      .WillOnce(Return(std::vector<kagome::primitives::BlockHash>{
          "EC"_H, "EB"_H, "EA"_H, "E"_H, "D"_H}));

  voting_round_->onPrevote(bob_vote);

  // then 1.
  ASSERT_EQ(voting_round_->getCurrentState().prevote_ghost.value(),
            Prevote(6, "E"_H));

  // then 2.
  ASSERT_TRUE(voting_round_->getCurrentState().estimate);
  ASSERT_EQ(voting_round_->getCurrentState().estimate.value(),
            BlockInfo(6, "E"_H));
  ASSERT_FALSE(voting_round_->completable());

  // when 3.
  // Eve prevotes
  auto eve_vote = preparePrevote(kEve, kEveSignature, Prevote{7, "F"_H});

  voting_round_->onPrevote(eve_vote);

  // then 3.
  ASSERT_EQ(voting_round_->getCurrentState().prevote_ghost.value(),
            Prevote(6, "E"_H));
  ASSERT_EQ(voting_round_->getCurrentState().estimate.value(),
            BlockInfo(6, "E"_H));
}

/**
 * @given Network of:
 * Alice with weight 4,
 * Bob with weight 7 and
 * Eve with weight 3
 * @and vote graph with base in Block with number 4 and hash "C"_h
 *
 * @when
 * 1. Alice precommits Precommit{10, "FC"_H}
 * 2. Bob precommits Precommit{10, "ED"_H}
 * 3. Alice prevotes Prevote{10, "FC"_H}
 * 4. Bob prevotes Prevote{10, "ED"_H}
 * 5. Eve prevotes Prevote{7, "EA"_H}
 * 6. Eve precommits Precommit{7, "EA"_H}
 *
 * @then
 * 1. After Bob precommits (when 2.) no finalized block exists as not enough
 * prevotes were collected
 * 2. After Bob prevotes (when 4.) we have finalized block BlockInfo(6, "E"_H)
 * which has supermajority on both prevotes and precommits
 * 3. After Eve prevotes (when 5.) we still have finalized == BlockInfo(6,
 * "E"_H)
 * 4. After Eve precommits (when 6.) finalized was updated to BlockInfo(7,
 * "EA"_H) (as this will become the highest block with supermajority)
 */
TEST_F(VotingRoundTest, Finalization) {
  // given (in fixture)

  // when 1.
  // Alice precommits FC
  EXPECT_CALL(*env_, getAncestry("C"_H, "FC"_H))
      .WillRepeatedly(Return(std::vector<kagome::primitives::BlockHash>{
          "FB"_H, "FA"_H, "F"_H, "E"_H, "D"_H}));

  voting_round_->onPrecommit(
      preparePrecommit(kAlice, kAliceSignature, {10, "FC"_H}));

  // when 2.
  // Bob precommits ED
  EXPECT_CALL(*env_, getAncestry("C"_H, "ED"_H))
      .WillOnce(Return(std::vector<kagome::primitives::BlockHash>{
          "EC"_H, "EB"_H, "EA"_H, "E"_H, "D"_H}));

  voting_round_->onPrecommit(
      preparePrecommit(kBob, kBobSignature, {10, "ED"_H}));

  // then 1.
  ASSERT_FALSE(voting_round_->getCurrentState().finalized.has_value());

  // import some prevotes.

  // when 3.
  // Alice Prevotes
  voting_round_->onPrevote(
      preparePrevote(kAlice, kAliceSignature, {10, "FC"_H}));

  // when 4.
  // Bob prevotes
  voting_round_->onPrevote(preparePrevote(kBob, kBobSignature, {10, "ED"_H}));
  // then 2.
  ASSERT_EQ(voting_round_->getCurrentState().finalized, BlockInfo(6, "E"_H));

  // when 5.
  // Eve prevotes
  voting_round_->onPrevote(preparePrevote(kEve, kEveSignature, {7, "EA"_H}));
  // then 3.
  ASSERT_EQ(voting_round_->getCurrentState().finalized, BlockInfo(6, "E"_H));

  // when 6.
  // Eve precommits
  voting_round_->onPrecommit(
      preparePrecommit(kEve, kEveSignature, {7, "EA"_H}));
  // then 3.
  ASSERT_EQ(voting_round_->getCurrentState().finalized, BlockInfo(7, "EA"_H));
}

ACTION_P(onProposed, test_fixture) {
  // immitating primary proposed is received from network
  test_fixture->voting_round_->onPrimaryPropose(arg2);
  return outcome::success();
}

ACTION_P(onPrevoted, test_fixture) {
  // immitate receiving prevotes from other peers
  auto signed_prevote = arg2;

  // send Alice's prevote
  test_fixture->voting_round_->onPrevote(signed_prevote);
  // send Bob's prevote
  test_fixture->voting_round_->onPrevote(
      SignedMessage{.message = signed_prevote.message,
                    .signature = test_fixture->kBobSignature,
                    .id = test_fixture->kBob});
  // send Eve's prevote
  test_fixture->voting_round_->onPrevote(
      SignedMessage{.message = signed_prevote.message,
                    .signature = test_fixture->kEveSignature,
                    .id = test_fixture->kEve});
  return outcome::success();
}

ACTION_P(onPrecommitted, test_fixture) {
  // immitate receiving precommit from other peers
  auto signed_precommit = arg2;

  // send Alice's precommit
  test_fixture->voting_round_->onPrecommit(signed_precommit);
  // send Bob's precommit
  test_fixture->voting_round_->onPrecommit(
      SignedMessage{.message = signed_precommit.message,
                    .signature = test_fixture->kBobSignature,
                    .id = test_fixture->kBob});
//  // send Eve's precommit
//  test_fixture->voting_round_->onPrecommit(
//      SignedMessage{.message = signed_precommit.message,
//                    .signature = test_fixture->kEveSignature,
//                    .id = test_fixture->kEve});
  return outcome::success();
}

ACTION_P(onFinalize, test_fixture) {
  kagome::consensus::grandpa::Fin fin{
      .round_number = arg0, .vote = arg1, .justification = arg2};
  test_fixture->voting_round_->onFinalize(fin);
  return outcome::success();
}

/**
 * Executes one round of grandpa round with mocked environment which mimics the
 * network of 3 nodes: Alice (current peer), Bob and Eve. Round is executed from
 * the Alice's perspective (so Bob's and Eve's behaviour is mocked)
 *
 * Scenario is the following:
 * @given
 * 1. Base block (last finalized one) in graph is BlockInfo{4, "C"_H}
 * 2. Best block (the one that Alice is trying to finalize) is BlockInfo{10,
 * "FC"_H}
 * 3. Last round state with:
 * prevote_ghost = Prevote{3, "B"_H}
 * estimate = BlockInfo{4, "C"_H}
 * finalized = BlockInfo{3, "B"_H}
 * 4. Peers:
 * Alice with weight 4 (primary),
 * Bob with weight 7 and
 * Eve with weight 3
 *
 * @when
 * The following test scenario is executed:
 * 1. Alice proposes BlockInfo{4, "C"_H} (last round's estimate)
 * 2. Everyone receive primary propose
 * 3. Alice prevotes Prevote{10, "FC"_H} which is the best chain containing
 * primary vote
 * 4. Everyone receive Prevote{10, "FC"_H} and send their prevotes for the same
 * block
 * 5. Alice precommits Precommit{10, "FC"_H} which is prevote_ghost for the
 * current round
 * 6. Everyone receive Precommit{10, "FC"_H} and send their precommit for the
 * same round
 * 7. Alice receives enough precommits to commit Precommit{10, "FC"_H}
 * 8. Round completes with round state containing `prevote_ghost`, `estimate`
 * and `finalized` equal to the best block that Alice voted for
 */
TEST_F(VotingRoundTest, SunnyDayScenario) {
  BlockHash base_block_hash = "C"_H;
  BlockNumber base_block_number = 4;
  BlockHash best_block_hash = "FC"_H;
  BlockNumber best_block_number = 10;

  // needed for Alice to get the best block to vote for
  EXPECT_CALL(*env_, bestChainContaining(base_block_hash))
      .WillRepeatedly(Return(BlockInfo{best_block_number, best_block_hash}));

  EXPECT_CALL(*env_, getAncestry(base_block_hash, best_block_hash))
      .WillRepeatedly(Return(std::vector<kagome::primitives::BlockHash>{
          "FB"_H, "FA"_H, "F"_H, "E"_H, "D"_H}));
  EXPECT_CALL(*env_, getAncestry(best_block_hash, best_block_hash))
      .WillRepeatedly(
          Return(std::vector<kagome::primitives::BlockHash>{best_block_hash}));

  RoundState last_round_state;
  last_round_state.prevote_ghost.emplace(3, "B"_H);
  last_round_state.estimate.emplace(base_block_number, base_block_hash);
  last_round_state.finalized.emplace(3, "B"_H);

  // voting round is executed by Alice. She is also a Primary as round number
  // is 0 and she is the first in voters set
  EXPECT_CALL(*env_,
              onProposed(_, _, Truly([&](const SignedMessage &primary_propose) {
                           std::cout << "Proposed: "
                                     << primary_propose.block_hash().data()
                                     << std::endl;
                           return primary_propose.block_hash()
                                      == last_round_state.estimate->block_hash
                                  and primary_propose.id == kAlice;
                         })))
      .WillOnce(onProposed(this));  // propose;

  EXPECT_CALL(*env_, onPrevoted(_, _, Truly([&](const SignedMessage &prevote) {
                                  std::cout << "Prevoted: "
                                            << prevote.block_hash().data()
                                            << std::endl;
                                  return prevote.block_hash() == best_block_hash
                                         and prevote.id == kAlice;
                                })))
      .WillOnce(onPrevoted(this));  // prevote;

  EXPECT_CALL(*env_,
              onPrecommitted(_, _, Truly([&](const SignedMessage &precommit) {
                               std::cout << "Precommited: "
                                         << precommit.block_hash().data()
                                         << std::endl;
                               return precommit.block_hash() == best_block_hash
                                      and precommit.id == kAlice;
                             })))
      .WillOnce(onPrecommitted(this));  // precommit;

  // check that expected fin message was sent
  EXPECT_CALL(
      *env_,
      onCommitted(
          round_number_,
          Truly([&](const BlockInfo &compared) {
            std::cout << "Finalized: " << compared.block_hash.data()
                      << std::endl;
            return compared == BlockInfo{best_block_number, best_block_hash};
          }),
          Truly([&](GrandpaJustification just) {
            Precommit precommit{best_block_number, best_block_hash};
            auto alice_precommit =
                preparePrecommit(kAlice, kAliceSignature, precommit);
            auto bob_precommit =
                preparePrecommit(kBob, kBobSignature, precommit);
            bool has_alice_precommit =
                boost::find(just.items, alice_precommit) != just.items.end();
            bool has_bob_precommit =
                boost::find(just.items, bob_precommit) != just.items.end();

            return has_alice_precommit and has_bob_precommit;
          })))
      .WillOnce(onFinalize(this));

  EXPECT_CALL(*env_, finalize(best_block_hash, _))
      .WillOnce(Return(outcome::success()));

  // check if completed round is as expected
  Prevote expected_prevote_ghost{best_block_number, best_block_hash};
  BlockInfo expected_final_estimate{best_block_number, best_block_hash};
  RoundState expected_state{.prevote_ghost = expected_prevote_ghost,
                            .estimate = expected_final_estimate,
                            .finalized = expected_final_estimate};
  CompletedRound expected_completed_round{.round_number = round_number_,
                                          .state = expected_state};
  EXPECT_CALL(
      *env_,
      onCompleted(outcome::result<CompletedRound>(expected_completed_round)))
      .WillRepeatedly(Return());

  voting_round_->primaryPropose(last_round_state);
  voting_round_->prevote(last_round_state);
  voting_round_->precommit(last_round_state);

  io_context_->run_for(duration_ * 6);
}

/**
 * @given last round state with no estimate
 * @when start voting round by calling primary propose, prevote and precommit
 * @then round is stopped by calling on completed with
 * NO_ESTIMATE_FOR_PREVIOUS_ROUND error
 */
TEST_F(VotingRoundTest, NoEstimateForPreviousRound) {
  // given
  RoundState last_round_state;
  // next line is redundant, but left just to explicitly state given conditions
  // of the test
  last_round_state.estimate = boost::none;

  // then
  EXPECT_CALL(*env_,
              onCompleted(outcome::result<CompletedRound>(
                  VotingRoundError::NO_ESTIMATE_FOR_PREVIOUS_ROUND)))
      .WillOnce(Return());

  // when
  voting_round_->primaryPropose(last_round_state);
  voting_round_->prevote(last_round_state);
  voting_round_->precommit(last_round_state);

  io_context_->run_for(duration_ * 6);
}
