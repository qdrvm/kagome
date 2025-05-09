/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/voting_round_impl.hpp"

#include <gtest/gtest.h>

#include <mock/libp2p/basic/scheduler_mock.hpp>

#include "consensus/grandpa/common.hpp"
#include "consensus/grandpa/grandpa_config.hpp"
#include "consensus/grandpa/impl/vote_tracker_impl.hpp"
#include "consensus/grandpa/vote_graph/vote_graph_impl.hpp"
#include "core/consensus/grandpa/literals.hpp"
#include "mock/core/consensus/grandpa/environment_mock.hpp"
#include "mock/core/consensus/grandpa/grandpa_mock.hpp"
#include "mock/core/consensus/grandpa/vote_crypto_provider_mock.hpp"
#include "mock/core/consensus/grandpa/voting_round_mock.hpp"
#include "mock/core/crypto/hasher_mock.hpp"
#include "mock/core/runtime/grandpa_api_mock.hpp"
#include "testutil/prepare_loggers.hpp"

using namespace kagome::consensus::grandpa;
using kagome::consensus::grandpa::Authority;
using kagome::consensus::grandpa::AuthoritySet;
using kagome::consensus::grandpa::Equivocation;
using kagome::consensus::grandpa::HistoricalVotes;
using kagome::crypto::Ed25519Keypair;
using kagome::crypto::Ed25519Signature;
using kagome::crypto::HasherMock;
using kagome::runtime::GrandpaApiMock;
using Propagation = kagome::consensus::grandpa::VotingRound::Propagation;

using namespace std::chrono_literals;

using testing::_;
using testing::AnyNumber;
using testing::Invoke;
using testing::Ref;
using testing::Return;
using testing::ReturnRef;
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
    EXPECT_CALL(*grandpa_, tryExecuteNextRound(_)).Times(AnyNumber());
    EXPECT_CALL(*grandpa_, updateNextRound(_)).Times(AnyNumber());

    auto authorities = std::make_shared<AuthoritySet>();
    authorities->id = 0;
    authorities->authorities.emplace_back(Authority{{kAlice}, kAliceWeight});
    authorities->authorities.emplace_back(Authority{{kBob}, kBobWeight});
    authorities->authorities.emplace_back(Authority{{kEve}, kEveWeight});

    auto voters = VoterSet::make(*authorities).value();

    GrandpaConfig config{.voters = std::move(voters),
                         .round_number = round_number_,
                         .duration = duration_,
                         .id = kAlice};

    env_ = std::make_shared<EnvironmentMock>();
    EXPECT_CALL(*env_, getAncestry("C"_H, "EA"_H))
        .WillRepeatedly(
            Return(std::vector<BlockHash>{"EA"_H, "E"_H, "D"_H, "C"_H}));
    EXPECT_CALL(*env_, getAncestry("C"_H, "FC"_H))
        .WillRepeatedly(Return(std::vector<BlockHash>{
            "FC"_H, "FB"_H, "FA"_H, "F"_H, "E"_H, "D"_H, "C"_H}));
    EXPECT_CALL(*env_, getAncestry("C"_H, "ED"_H))
        .WillRepeatedly(Return(std::vector<BlockHash>{
            "ED"_H, "EC"_H, "EB"_H, "EA"_H, "E"_H, "D"_H, "C"_H}));
    EXPECT_CALL(*env_, hasAncestry("C"_H, "FC"_H)).WillRepeatedly(Return(true));
    EXPECT_CALL(*env_, hasAncestry("E"_H, "ED"_H)).WillRepeatedly(Return(true));
    EXPECT_CALL(*env_, hasAncestry("E"_H, "FC"_H)).WillRepeatedly(Return(true));
    EXPECT_CALL(*env_, hasAncestry("EA"_H, "EA"_H))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*env_, hasAncestry("EA"_H, "FC"_H))
        .WillRepeatedly(Return(false));
    EXPECT_CALL(*env_, hasAncestry("EA"_H, "ED"_H))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*env_, hasAncestry("FC"_H, "FC"_H))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*env_, bestChainContaining("C"_H, _))
        .WillRepeatedly(Return(BlockInfo{9, "FC"_H}));

    vote_graph_ = std::make_shared<VoteGraphImpl>(base, config.voters, env_);

    scheduler_ = std::make_shared<libp2p::basic::SchedulerMock>();
    EXPECT_CALL(*scheduler_, scheduleImpl(_, _, _)).Times(AnyNumber());
    EXPECT_CALL(*scheduler_, now()).Times(AnyNumber());

    previous_round_ = std::make_shared<VotingRoundMock>();
    ON_CALL(*previous_round_, lastFinalizedBlock())
        .WillByDefault(Return(BlockInfo{0, "genesis"_H}));
    EXPECT_CALL(*previous_round_, bestFinalCandidate())
        .Times(AnyNumber())
        .WillRepeatedly(Return(BlockInfo{3, "C"_H}));
    EXPECT_CALL(*previous_round_, attemptToFinalizeRound())
        .Times(AnyNumber())
        .WillRepeatedly(Return());
    EXPECT_CALL(*previous_round_, finalizedBlock())
        .Times(AnyNumber())
        .WillRepeatedly(ReturnRef(finalized_in_prev_round_));
    EXPECT_CALL(*previous_round_, doCommit()).Times(AnyNumber());

    round_ = std::make_shared<VotingRoundImpl>(grandpa_,
                                               config,
                                               hasher_,
                                               env_,
                                               vote_crypto_provider_,
                                               prevotes_,
                                               precommits_,
                                               vote_graph_,
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
  std::shared_ptr<EnvironmentMock> env_;
  std::shared_ptr<VoteGraphImpl> vote_graph_;

  std::shared_ptr<libp2p::basic::SchedulerMock> scheduler_;

  const std::optional<BlockInfo> finalized_in_prev_round_{{2, "B"_H}};
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
  round_->onPrevote({}, alice_vote, Propagation::NEEDLESS);
  round_->update(VotingRound::IsPreviousRoundChanged{false},
                 VotingRound::IsPrevotesChanged{true},
                 VotingRound::IsPrecommitsChanged{false});

  // Bob prevotes
  auto bob_vote = preparePrevote(kBob, kBobSignature, Prevote{9, "ED"_H});
  round_->onPrevote({}, bob_vote, Propagation::NEEDLESS);
  round_->update(VotingRound::IsPreviousRoundChanged{false},
                 VotingRound::IsPrevotesChanged{true},
                 VotingRound::IsPrecommitsChanged{false});

  // then 1.
  ASSERT_EQ(round_->bestFinalCandidate(), BlockInfo(5, "E"_H));
  ASSERT_FALSE(round_->completable());

  // when 2.
  // Eve prevotes
  auto eve_vote = preparePrevote(kEve, kEveSignature, Prevote{6, "F"_H});

  round_->onPrevote({}, eve_vote, Propagation::NEEDLESS);
  round_->update(VotingRound::IsPreviousRoundChanged{false},
                 VotingRound::IsPrevotesChanged{true},
                 VotingRound::IsPrecommitsChanged{false});

  // then 2.
  ASSERT_EQ(round_->bestFinalCandidate(), BlockInfo(5, "E"_H));
}

TEST_F(VotingRoundTest, EquivocateDoesNotDoubleCount) {
  auto alice1 = preparePrevote(kAlice, kAliceSignature, Prevote{9, "FC"_H});
  auto alice2 = preparePrevote(kAlice, kAliceSignature, Prevote{9, "ED"_H});
  auto alice3 = preparePrevote(kAlice, kAliceSignature, Prevote{6, "F"_H});

  Equivocation equivocation{round_->roundNumber(), alice1, alice2};

  {
    auto matcher = [&](const Equivocation &equivocation) {
      auto &first = equivocation.first;
      auto &second = equivocation.second;

      if (equivocation.offender() != kAlice) {
        return false;
      }
      if (first.id != equivocation.offender()
          or second.id != equivocation.offender()) {
        return false;
      }
      if (not first.is<Prevote>() or not second.is<Prevote>()) {
        return false;
      }
      std::cout << "Equivocation: "  //
                << "first vote for " << first.getBlockHash().data() << ", "
                << "second vote for " << second.getBlockHash().data()
                << std::endl;
      return true;
    };

    EXPECT_CALL(*env_, reportEquivocation(_, Truly(matcher)))
        .WillOnce(Return(outcome::success()));
  }

  // Regular vote
  round_->onPrevote({}, alice1, Propagation::NEEDLESS);

  // Different vote in the same round; equivocation must be reported
  round_->onPrevote({}, alice2, Propagation::NEEDLESS);

  // Another vote in the same round; should be ignored, cause already reported
  round_->onPrevote({}, alice3, Propagation::NEEDLESS);

  round_->update(IsPreviousRoundChanged{false},
                 IsPrevotesChanged{true},
                 IsPrecommitsChanged{false});

  ASSERT_EQ(round_->prevoteGhost(), std::nullopt);
  auto bob = preparePrevote(kBob, kBobSignature, Prevote{7, "FA"_H});
  round_->onPrevote({}, bob, Propagation::NEEDLESS);
  round_->update(IsPreviousRoundChanged{false},
                 IsPrevotesChanged{true},
                 IsPrecommitsChanged{false});
  ASSERT_EQ(round_->prevoteGhost(), (BlockInfo{7, "FA"_H}));
}

// https://github.com/paritytech/finality-grandpa/blob/8c45a664c05657f0c71057158d3ba555ba7d20de/src/round.rs#L844
TEST_F(VotingRoundTest, HistoricalVotesWorks) {
  auto alice1 = preparePrevote(kAlice, kAliceSignature, Prevote{9, "FC"_H});
  auto alice2 = preparePrevote(kAlice, kAliceSignature, Prevote{9, "EC"_H});
  auto bob1 = preparePrevote(kBob, kBobSignature, Prevote{7, "EA"_H});
  auto bob2 = preparePrecommit(kBob, kBobSignature, Precommit{7, "EA"_H});

  EXPECT_CALL(*env_, reportEquivocation(_, _))
      .WillOnce(Return(outcome::success()));
  EXPECT_CALL(*grandpa_,
              saveHistoricalVote(
                  round_->voterSetId(), round_->roundNumber(), alice1, true));
  round_->onPrevote({}, alice1, Propagation::NEEDLESS);
  EXPECT_CALL(*grandpa_,
              saveHistoricalVote(
                  round_->voterSetId(), round_->roundNumber(), bob1, false));
  round_->onPrevote({}, bob1, Propagation::NEEDLESS);
  EXPECT_CALL(*grandpa_,
              saveHistoricalVote(
                  round_->voterSetId(), round_->roundNumber(), bob2, false));
  round_->onPrecommit({}, bob2, Propagation::NEEDLESS);
  EXPECT_CALL(*grandpa_,
              saveHistoricalVote(
                  round_->voterSetId(), round_->roundNumber(), alice2, true));
  round_->onPrevote({}, alice2, Propagation::NEEDLESS);
  round_->update(IsPreviousRoundChanged{false},
                 IsPrevotesChanged{true},
                 IsPrecommitsChanged{true});
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
  EXPECT_CALL(*env_, finalize(_, _)).WillRepeatedly(Return(outcome::success()));
  // given (in fixture)

  // when 1.
  // Alice Prevotes FC
  auto alice_prevote = preparePrevote(kAlice, kAliceSignature, {9, "FC"_H});
  round_->onPrevote({}, alice_prevote, Propagation::NEEDLESS);
  round_->update(VotingRound::IsPreviousRoundChanged{false},
                 VotingRound::IsPrevotesChanged{true},
                 VotingRound::IsPrecommitsChanged{false});

  // when 2.
  // Bob prevotes ED
  auto bob_prevote = preparePrevote(kBob, kBobSignature, {9, "ED"_H});
  round_->onPrevote({}, bob_prevote, Propagation::NEEDLESS);
  round_->update(VotingRound::IsPreviousRoundChanged{false},
                 VotingRound::IsPrevotesChanged{true},
                 VotingRound::IsPrecommitsChanged{false});

  // then 1.
  ASSERT_FALSE(round_->finalizedBlock().has_value());

  // import some prevotes.

  // when 3.
  // Alice precommits FC
  auto alice_precommit = preparePrecommit(kAlice, kAliceSignature, {9, "FC"_H});
  round_->onPrecommit({}, alice_precommit, Propagation::NEEDLESS);
  round_->update(VotingRound::IsPreviousRoundChanged{false},
                 VotingRound::IsPrevotesChanged{false},
                 VotingRound::IsPrecommitsChanged{true});

  // when 4.
  // Bob precommits ED
  auto bob_precommit = preparePrecommit(kBob, kBobSignature, {9, "ED"_H});
  round_->onPrecommit({}, bob_precommit, Propagation::NEEDLESS);
  round_->update(VotingRound::IsPreviousRoundChanged{false},
                 VotingRound::IsPrevotesChanged{false},
                 VotingRound::IsPrecommitsChanged{true});

  // then 2.
  ASSERT_EQ(round_->finalizedBlock(), BlockInfo(5, "E"_H));

  // when 5.
  // Eve prevotes
  round_->onPrevote({},
                    preparePrevote(kEve, kEveSignature, {6, "EA"_H}),
                    Propagation::NEEDLESS);
  round_->update(VotingRound::IsPreviousRoundChanged{false},
                 VotingRound::IsPrevotesChanged{true},
                 VotingRound::IsPrecommitsChanged{false});
  // then 3.
  ASSERT_EQ(round_->finalizedBlock(), BlockInfo(5, "E"_H));

  // when 6.
  // Eve precommits
  round_->onPrecommit({},
                      preparePrecommit(kEve, kEveSignature, {6, "EA"_H}),
                      Propagation::NEEDLESS);
  round_->update(VotingRound::IsPreviousRoundChanged{false},
                 VotingRound::IsPrevotesChanged{false},
                 VotingRound::IsPrecommitsChanged{true});

  // then 3.
  ASSERT_EQ(round_->finalizedBlock(), BlockInfo(6, "EA"_H));
}

ACTION_P(onProposed, test_fixture) {
  // imitating primary proposed is received from network
  test_fixture->round_->onProposal({}, arg2, Propagation::NEEDLESS);
}

ACTION_P(onPrevoted, test_fixture) {
  // imitate receiving prevotes from other peers
  auto signed_prevote = arg2;

  // send Alice's prevote
  test_fixture->round_->onPrevote({}, signed_prevote, Propagation::NEEDLESS);
  // send Bob's prevote
  test_fixture->round_->onPrevote(
      {},
      SignedMessage{.message = signed_prevote.message,
                    .signature = test_fixture->kBobSignature,
                    .id = test_fixture->kBob},
      Propagation::NEEDLESS);
  // send Eve's prevote
  test_fixture->round_->onPrevote(
      {},
      SignedMessage{.message = signed_prevote.message,
                    .signature = test_fixture->kEveSignature,
                    .id = test_fixture->kEve},
      Propagation::NEEDLESS);
  test_fixture->round_->update(VotingRound::IsPreviousRoundChanged{false},
                               VotingRound::IsPrevotesChanged{true},
                               VotingRound::IsPrecommitsChanged{false});
}

ACTION_P(onPrecommitted, test_fixture) {
  // imitate receiving precommit from other peers
  auto signed_precommit = arg2;

  // send Alice's precommit
  test_fixture->round_->onPrecommit(
      {}, signed_precommit, Propagation::NEEDLESS);
  // send Bob's precommit
  test_fixture->round_->onPrecommit(
      {},
      SignedMessage{.message = signed_precommit.message,
                    .signature = test_fixture->kBobSignature,
                    .id = test_fixture->kBob},
      Propagation::NEEDLESS);
  //  // send Eve's precommit
  //  test_fixture->round_->onPrecommit(
  //      SignedMessage{.message = signed_precommit.message,
  //                    .signature = test_fixture->kEveSignature,
  //                    .id = test_fixture->kEve}, Propagation::NEEDLESS);
  test_fixture->round_->update(VotingRound::IsPreviousRoundChanged{false},
                               VotingRound::IsPrevotesChanged{false},
                               VotingRound::IsPrecommitsChanged{true});
}

ACTION_P(onFinalize, test_fixture) {
  (void)test_fixture->env_->finalize(0, arg2);
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

  round_->play();
  round_->endPrevoteStage();
  round_->endPrecommitStage();

  auto state = round_->state();

  Precommit precommit{best_block.number, best_block.hash};

  auto alice_precommit = preparePrecommit(kAlice, kAliceSignature, precommit);
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

  EXPECT_TRUE(has_alice_precommit);
  EXPECT_TRUE(has_bob_precommit);

  ASSERT_TRUE(state.finalized.has_value());
  EXPECT_EQ(state.finalized.value(), best_block);
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
TEST_F(VotingRoundTest, Equivocation) {
  EXPECT_CALL(*env_, finalize(_, _))
      .Times(AnyNumber())
      .WillRepeatedly(Return(outcome::success()));
  auto base_block = previous_round_->bestFinalCandidate();

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

  round_->play();
  round_->endPrevoteStage();
  round_->endPrecommitStage();

  auto state = round_->state();

  Precommit precommit{best_block.number, best_block.hash};

  auto alice_precommit = preparePrecommit(kAlice, kAliceSignature, precommit);
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

  EXPECT_TRUE(has_alice_precommit);
  EXPECT_TRUE(has_bob_precommit);

  ASSERT_TRUE(state.finalized.has_value());
  EXPECT_EQ(state.finalized.value(), best_block);
}
