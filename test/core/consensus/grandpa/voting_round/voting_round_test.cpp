/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <mock/libp2p/basic/scheduler_mock.hpp>

#include "clock/impl/clock_impl.hpp"
#include "common/visitor.hpp"
#include "consensus/grandpa/impl/vote_tracker_impl.hpp"
#include "consensus/grandpa/impl/voting_round_error.hpp"
#include "consensus/grandpa/impl/voting_round_impl.hpp"
#include "consensus/grandpa/vote_graph/vote_graph_impl.hpp"
#include "core/consensus/grandpa/literals.hpp"
#include "mock/core/consensus/authority/authority_manager_mock.hpp"
#include "mock/core/consensus/grandpa/environment_mock.hpp"
#include "mock/core/consensus/grandpa/grandpa_mock.hpp"
#include "mock/core/consensus/grandpa/vote_crypto_provider_mock.hpp"
#include "mock/core/consensus/grandpa/voting_round_mock.hpp"
#include "mock/core/crypto/hasher_mock.hpp"
#include "primitives/authority.hpp"
#include "testutil/outcome.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace kagome::consensus::grandpa;
using namespace std::chrono_literals;

using kagome::authority::AuthorityManagerMock;
using kagome::clock::SteadyClockImpl;
using kagome::consensus::grandpa::EnvironmentMock;
using kagome::consensus::grandpa::GrandpaConfig;
using kagome::consensus::grandpa::VoteCryptoProviderMock;
using kagome::consensus::grandpa::VotingRound;
using kagome::crypto::Ed25519Keypair;
using kagome::crypto::Ed25519Signature;
using kagome::crypto::HasherMock;
using kagome::primitives::Authority;
using kagome::primitives::AuthorityList;
using Propagation = kagome::consensus::grandpa::VotingRound::Propagation;
using kagome::visit_in_place;

using testing::_;
using testing::AnyNumber;
using testing::Invoke;
using testing::Return;
using testing::Truly;

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

class VotingRoundTest : public testing::Test,
                        // Next inheritance only for access to private methods
                        protected VotingRoundImpl {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
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

    BlockInfo base{3, "C"_H};

    grandpa_ = std::make_shared<GrandpaMock>();
    EXPECT_CALL(*grandpa_, executeNextRound()).Times(AnyNumber());

    auto authorities = std::make_shared<AuthorityList>();
    authorities->id = 0;
    authorities->emplace_back(Authority{{kAlice}, kAliceWeight});
    authorities->emplace_back(Authority{{kBob}, kBobWeight});
    authorities->emplace_back(Authority{{kEve}, kEveWeight});

    authority_manager_ = std::make_shared<AuthorityManagerMock>();
    EXPECT_CALL(*authority_manager_, base())
        .Times(AnyNumber())
        .WillRepeatedly(Return(BlockInfo{2, "B"_H}));
    EXPECT_CALL(*authority_manager_, authorities(_, _))
        .Times(AnyNumber())
        .WillRepeatedly(Return(authorities));

    auto voters = std::make_shared<VoterSet>(authorities->id);
    for (const auto &authority : *authorities) {
      ASSERT_OUTCOME_SUCCESS_TRY(
          voters->insert(kagome::primitives::GrandpaSessionKey(authority.id.id),
                         authority.weight));
    }

    GrandpaConfig config{.voters = std::move(voters),
                         .round_number = round_number_,
                         .duration = duration_,
                         .id = kAlice};

    env_ = std::make_shared<EnvironmentMock>();
    EXPECT_CALL(*env_, getAncestry("C"_H, "FC"_H))
        .WillRepeatedly(Return(std::vector<BlockHash>{
            "FC"_H, "FB"_H, "FA"_H, "F"_H, "E"_H, "D"_H, "C"_H}));
    EXPECT_CALL(*env_, getAncestry("C"_H, "ED"_H))
        .WillRepeatedly(Return(std::vector<BlockHash>{
            "ED"_H, "EC"_H, "EB"_H, "EA"_H, "E"_H, "D"_H, "C"_H}));
    EXPECT_CALL(*env_, hasAncestry("C"_H, "FC"_H)).WillRepeatedly(Return(true));
    EXPECT_CALL(*env_, hasAncestry("E"_H, "ED"_H)).WillRepeatedly(Return(true));
    EXPECT_CALL(*env_, hasAncestry("E"_H, "FC"_H)).WillRepeatedly(Return(true));
    EXPECT_CALL(*env_, hasAncestry("EA"_H, "FC"_H))
        .WillRepeatedly(Return(false));
    EXPECT_CALL(*env_, hasAncestry("EA"_H, "ED"_H))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*env_, hasAncestry("FC"_H, "FC"_H))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*env_, bestChainContaining("C"_H))
        .WillRepeatedly(Return(BlockInfo{9, "FC"_H}));
    EXPECT_CALL(*env_, onNeighborMessageSent(_, _, _))
        .WillRepeatedly(Return(outcome::success()));

    prevote_graph_ = std::make_shared<VoteGraphImpl>(base, config.voters, env_);
    precommit_graph_ =
        std::make_shared<VoteGraphImpl>(base, config.voters, env_);

    scheduler_ = std::make_shared<libp2p::basic::SchedulerMock>();
    EXPECT_CALL(*scheduler_, scheduleImplMockCall(_, _, _)).Times(AnyNumber());
    EXPECT_CALL(*scheduler_, nowMockCall()).Times(AnyNumber());

    previous_round_ = std::make_shared<VotingRoundMock>();
    ON_CALL(*previous_round_, lastFinalizedBlock())
        .WillByDefault(Return(BlockInfo{0, "genesis"_H}));
    ON_CALL(*previous_round_, bestPrevoteCandidate())
        .WillByDefault(Return(BlockInfo{2, "B"_H}));
    ON_CALL(*previous_round_, bestPrecommitCandidate())
        .WillByDefault(Return(BlockInfo{3, "C"_H}));
    EXPECT_CALL(*previous_round_, bestFinalCandidate())
        .Times(AnyNumber())
        .WillRepeatedly(Return(BlockInfo{3, "C"_H}));
    EXPECT_CALL(*previous_round_, attemptToFinalizeRound())
        .Times(AnyNumber())
        .WillRepeatedly(Return());
    EXPECT_CALL(*previous_round_, finalizedBlock())
        .Times(AnyNumber())
        .WillRepeatedly(Return(BlockInfo{2, "B"_H}));
    ON_CALL(*previous_round_, doCommit()).WillByDefault(Return());

    round_ = std::make_shared<VotingRoundImpl>(grandpa_,
                                               config,
                                               authority_manager_,
                                               env_,
                                               vote_crypto_provider_,
                                               prevotes_,
                                               precommits_,
                                               prevote_graph_,
                                               precommit_graph_,
                                               clock_,
                                               scheduler_,
                                               previous_round_);
  }

  SignedMessage preparePrimaryPropose(const Id &id,
                                      const Ed25519Signature &sig,
                                      const PrimaryPropose &primary_propose) {
    return SignedMessage{
        .message = primary_propose, .signature = sig, .id = id};
  }

  SignedMessage preparePrevote(const Id &id,
                               const Ed25519Signature &sig,
                               const Prevote &prevote) {
    return SignedMessage{.message = prevote, .signature = sig, .id = id};
  }

  SignedMessage preparePrecommit(const Id &id,
                                 const Ed25519Signature &sig,
                                 const Precommit &precommit) {
    return SignedMessage{.message = precommit, .signature = sig, .id = id};
  }

 public:
  const BlockHash GENESIS_HASH = "genesis"_H;

  const Id kAlice = "Alice"_ID;
  const size_t kAliceWeight = 4;
  const Ed25519Signature kAliceSignature = "Alice"_SIG;

  const Id kBob = "Bob"_ID;
  const size_t kBobWeight = 7;
  const Ed25519Signature kBobSignature = "Bob"_SIG;

  const Id kEve = "Eve"_ID;
  const size_t kEveWeight = 3;
  const Ed25519Signature kEveSignature = "Eve"_SIG;

  RoundNumber round_number_{0};
  static constexpr auto duration_ = 100ms;
  TimePoint start_time_{42h};

  Ed25519Keypair keypair_;
  std::shared_ptr<VoteCryptoProviderMock> vote_crypto_provider_ =
      std::make_shared<VoteCryptoProviderMock>();

  std::shared_ptr<HasherMock> hasher_ = std::make_shared<HasherMock>();

  std::shared_ptr<VoteTrackerImpl> prevotes_ =
      std::make_shared<VoteTrackerImpl>();
  std::shared_ptr<VoteTrackerImpl> precommits_ =
      std::make_shared<VoteTrackerImpl>();

  std::shared_ptr<GrandpaMock> grandpa_;
  std::shared_ptr<AuthorityManagerMock> authority_manager_;
  std::shared_ptr<EnvironmentMock> env_;
  std::shared_ptr<VoteGraphImpl> prevote_graph_;
  std::shared_ptr<VoteGraphImpl> precommit_graph_;
  std::shared_ptr<Clock> clock_ = std::make_shared<SteadyClockImpl>();

  std::shared_ptr<libp2p::basic::SchedulerMock> scheduler_;

  std::shared_ptr<VotingRoundMock> previous_round_;
  std::shared_ptr<VotingRoundImpl> round_;
};

/**
 * # 0   1   2   3   4   5      6     7    8    9
 *
 *                                 - FA - FB - FC
 *                               /
 * GEN - A - B - C - D - E +--- F
 *                          \                    .
 *                           \
 *                            - EA - EB - EC - ED
 *
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
 * @and vote graph with base in Block with number 3 and hash "C"_h
 * @when
 * 1. Alice prevotes Prevote{9, "FC"_h}
 * 2. Bob prevotes Prevote{9, "ED"_h}
 * 3. Eve prevotes Prevote{6 , "F"_h}
 * @then
 * 1. After Bob prevoted, prevote_ghost will be Prevote{5, "E"_h} (as this is
 * the best ancestor of "FC"_h and "ED"_h with supermajority)
 * 2. After Bob prevoted, estimate will be BlockInfo{5, "E"_h} (as this is the
 * best block that can be finalized)
 * 3. After Eve prevotes, prevote_ghost and estimate should not change (as her
 * weight is not enough to have supermajority for "F"_h)
 */
TEST_F(VotingRoundTest, EstimateIsValid) {
  // given (in fixture)

  // when 1.
  // Alice prevotes
  auto alice_vote = preparePrevote(kAlice, kAliceSignature, Prevote{9, "FC"_H});
  round_->onPrevote(alice_vote, Propagation::NEEDLESS);
  round_->update(true, false);

  // when 2.
  // Bob prevotes
  auto bob_vote = preparePrevote(kBob, kBobSignature, Prevote{9, "ED"_H});
  round_->onPrevote(bob_vote, Propagation::NEEDLESS);
  round_->update(true, false);

  // then 1.
  ASSERT_EQ(round_->bestPrecommitCandidate(), BlockInfo(5, "E"_H));

  // then 2.
  ASSERT_EQ(round_->bestFinalCandidate(), BlockInfo(5, "E"_H));
  ASSERT_FALSE(round_->completable());

  // when 3.
  // Eve prevotes
  auto eve_vote = preparePrevote(kEve, kEveSignature, Prevote{6, "F"_H});

  round_->onPrevote(eve_vote, Propagation::NEEDLESS);
  round_->update(true, false);

  // then 3.
  ASSERT_EQ(round_->bestPrecommitCandidate(), BlockInfo(5, "E"_H));
  ASSERT_EQ(round_->bestFinalCandidate(), BlockInfo(5, "E"_H));
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
  EXPECT_CALL(*env_, onCommitted(_, _, _))
      .WillRepeatedly(Return(outcome::success()));
  EXPECT_CALL(*env_, onCompleted(_)).WillRepeatedly(Return());
  EXPECT_CALL(*env_, finalize(_, _)).WillRepeatedly(Return(outcome::success()));
  // given (in fixture)

  // when 1.
  // Alice Prevotes FC
  auto alice_prevote = preparePrevote(kAlice, kAliceSignature, {9, "FC"_H});
  round_->onPrevote(alice_prevote, Propagation::NEEDLESS);
  round_->update(true, false);

  // when 2.
  // Bob prevotes ED
  auto bob_prevote = preparePrevote(kBob, kBobSignature, {9, "ED"_H});
  round_->onPrevote(bob_prevote, Propagation::NEEDLESS);
  round_->update(true, false);

  // then 1.
  ASSERT_FALSE(round_->finalizedBlock().has_value());

  // import some prevotes.

  // when 3.
  // Alice precommits FC
  auto alice_precommit = preparePrecommit(kAlice, kAliceSignature, {9, "FC"_H});
  round_->onPrecommit(alice_precommit, Propagation::NEEDLESS);
  round_->update(false, true);

  // when 4.
  // Bob precommits ED
  auto bob_precommit = preparePrecommit(kBob, kBobSignature, {9, "ED"_H});
  round_->onPrecommit(bob_precommit, Propagation::NEEDLESS);
  round_->update(false, true);

  // then 2.
  ASSERT_EQ(round_->finalizedBlock(), BlockInfo(5, "E"_H));

  // when 5.
  // Eve prevotes
  round_->onPrevote(preparePrevote(kEve, kEveSignature, {6, "EA"_H}),
                    Propagation::NEEDLESS);
  round_->update(true, false);
  // then 3.
  ASSERT_EQ(round_->finalizedBlock(), BlockInfo(5, "E"_H));

  // when 6.
  // Eve precommits
  round_->onPrecommit(preparePrecommit(kEve, kEveSignature, {6, "EA"_H}),
                      Propagation::NEEDLESS);
  round_->update(false, true);

  // then 3.
  ASSERT_EQ(round_->finalizedBlock(), BlockInfo(6, "EA"_H));
}

ACTION_P(onProposed, test_fixture) {
  // imitating primary proposed is received from network
  test_fixture->round_->onProposal(arg2, Propagation::NEEDLESS);
  return outcome::success();
}

ACTION_P(onPrevoted, test_fixture) {
  // imitate receiving prevotes from other peers
  auto signed_prevote = arg2;

  // send Alice's prevote
  test_fixture->round_->onPrevote(signed_prevote, Propagation::NEEDLESS);
  // send Bob's prevote
  test_fixture->round_->onPrevote(
      SignedMessage{.message = signed_prevote.message,
                    .signature = test_fixture->kBobSignature,
                    .id = test_fixture->kBob},
      Propagation::NEEDLESS);
  // send Eve's prevote
  test_fixture->round_->onPrevote(
      SignedMessage{.message = signed_prevote.message,
                    .signature = test_fixture->kEveSignature,
                    .id = test_fixture->kEve},
      Propagation::NEEDLESS);
  test_fixture->round_->update(true, false);
  return outcome::success();
}

ACTION_P(onPrecommitted, test_fixture) {
  // imitate receiving precommit from other peers
  auto signed_precommit = arg2;

  // send Alice's precommit
  test_fixture->round_->onPrecommit(signed_precommit, Propagation::NEEDLESS);
  // send Bob's precommit
  test_fixture->round_->onPrecommit(
      SignedMessage{.message = signed_precommit.message,
                    .signature = test_fixture->kBobSignature,
                    .id = test_fixture->kBob},
      Propagation::NEEDLESS);
  //  // send Eve's precommit
  //  test_fixture->round_->onPrecommit(
  //      SignedMessage{.message = signed_precommit.message,
  //                    .signature = test_fixture->kEveSignature,
  //                    .id = test_fixture->kEve}, Propagation::NEEDLESS);
  test_fixture->round_->update(false, true);
  return outcome::success();
}

ACTION_P(onFinalize, test_fixture) {
  (void)test_fixture->env_->finalize(0, arg2);
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
  EXPECT_CALL(*env_, finalize(_, _))
      .Times(AnyNumber())
      .WillRepeatedly(Return(outcome::success()));
  auto base_block = previous_round_->bestFinalCandidate();
  auto finalized_block = *previous_round_->finalizedBlock();

  ASSERT_EQ(base_block, (BlockInfo{3, "C"_H}));

  BlockInfo best_block{9, "FC"_H};

  // Voting round is executed by Alice.
  // Alice is also a Primary (alice's voter index % round number is zero)
  {
    auto matcher = [&](const SignedMessage &primary_propose) {
      if (primary_propose.is<PrimaryPropose>() and primary_propose.id == kAlice
          and primary_propose.getBlockHash() == base_block.hash) {
        std::cout << "Proposed: " << primary_propose.getBlockHash().data()
                  << std::endl;
        return true;
      }
      return false;
    };
    EXPECT_CALL(*env_, onVoted(_, _, Truly(matcher)))
        .WillOnce(onProposed(this));  // propose;
  }

  // After prevote stage timer is out, Alice is doing prevote
  {
    auto matcher = [&](const SignedMessage &prevote) {
      if (prevote.is<Prevote>() and prevote.id == kAlice
          and prevote.getBlockHash() == best_block.hash) {
        std::cout << "Prevoted: " << prevote.getBlockHash().data() << std::endl;
        return true;
      }
      return false;
    };
    // Is doing prevote
    EXPECT_CALL(*env_, onVoted(_, _, Truly(matcher)))
        .WillOnce(onPrevoted(this));  // prevote;
  }

  // After precommit stage timer is out, Alice is doing precommit
  {
    auto matcher = [&](const SignedMessage &precommit) {
      if (precommit.is<Precommit>() and precommit.id == kAlice
          and precommit.getBlockHash() == best_block.hash) {
        std::cout << "Precommitted: " << precommit.getBlockHash().data()
                  << std::endl;
        return true;
      }
      return false;
    };
    // Is doing precommit
    EXPECT_CALL(*env_, onVoted(_, _, Truly(matcher)))
        .WillOnce(onPrecommitted(this));  // precommit;
  }

  {
    // check that round has completed
    auto matcher = [&](const outcome::result<MovableRoundState> &result) {
      if (not result.has_value()) {
        return false;
      }
      const auto &state = result.value();

      Precommit precommit{best_block.number, best_block.hash};

      auto alice_precommit =
          preparePrecommit(kAlice, kAliceSignature, precommit);
      auto bob_precommit = preparePrecommit(kBob, kBobSignature, precommit);

      bool has_alice_precommit = false;
      bool has_bob_precommit = false;

      auto lookup = [&](const auto &vote) {
        has_alice_precommit = vote == alice_precommit or has_alice_precommit;
        has_bob_precommit = vote == bob_precommit or has_bob_precommit;
      };

      for (auto &vote_variant : state.votes) {
        kagome::visit_in_place(
            vote_variant,
            [&](const SignedMessage &vote) { lookup(vote); },
            [&](const EquivocatorySignedMessage &pair) {
              lookup(pair.first);
              lookup(pair.second);
            });
      }

      // check if completed round state is as expected
      return state.round_number == round_number_
             and state.last_finalized_block == finalized_block
             and state.finalized.has_value()
             and state.finalized.value() == best_block and has_alice_precommit
             and has_bob_precommit;
    };

    EXPECT_CALL(*env_, onCompleted(Truly(matcher))).WillRepeatedly(Return());
  }

  round_->play();
  round_->endPrevoteStage();
  round_->endPrecommitStage();
}
