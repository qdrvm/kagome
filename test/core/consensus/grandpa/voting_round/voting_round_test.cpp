/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/voting_round_impl.hpp"

#include <gtest/gtest.h>
#include "core/consensus/grandpa/literals.hpp"
#include "mock/core/blockchain/block_tree_mock.hpp"
#include "mock/core/clock/clock_mock.hpp"
#include "mock/core/consensus/grandpa/chain_mock.hpp"
#include "mock/core/consensus/grandpa/gossiper_mock.hpp"
#include "mock/core/consensus/grandpa/vote_graph_mock.hpp"
#include "mock/core/consensus/grandpa/vote_tracker_mock.hpp"
#include "mock/core/crypto/ed25519_provider_mock.hpp"

using namespace kagome::consensus::grandpa;
using namespace std::chrono_literals;

using kagome::blockchain::BlockTreeMock;
using kagome::clock::SteadyClockMock;
using kagome::crypto::ED25519Keypair;
using kagome::crypto::ED25519ProviderMock;
using kagome::crypto::ED25519Signature;

using ::testing::Return;

class VotingRoundTest : public ::testing::Test {
 public:
  void SetUp() override {
    voters_->insert(kAlice, kAliceWeight);
    voters_->insert(kBob, kBobWeight);
    voters_->insert(kEve, kEveWeight);

    keypair_.public_key = kAlice;

    voting_round_ = std::make_shared<VotingRoundImpl>(voters_,
                                                      round_number_,
                                                      duration_,
                                                      start_time_,
                                                      counter_,
                                                      last_round_state_,
                                                      keypair_,
                                                      vote_tracker_,
                                                      vote_graph_,
                                                      gossiper_,
                                                      ed_provider_,
                                                      clock_,
                                                      tree_,
                                                      Timer(io_context_));
  }

  SignedPrevote preparePrevote(const Id &id,
                               const ED25519Signature &sig,
                               const Prevote &prevote) {
    return SignedPrevote{.id = id, .signature = sig, .message = prevote};
  }

  VoteWeight preparePrevoteWeight(const Id &id) {
    VoteWeight v(voters_->size());
    auto index = voters_->voterIndex(id);
    v.prevotes[index.value()] = true;
    return v;
  }

 protected:
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
  std::shared_ptr<VoteTrackerMock> vote_tracker_ =
      std::make_shared<VoteTrackerMock>();
  std::shared_ptr<VoteGraphMock> vote_graph_ =
      std::make_shared<VoteGraphMock>();
  std::shared_ptr<GossiperMock> gossiper_ = std::make_shared<GossiperMock>();
  std::shared_ptr<ED25519ProviderMock> ed_provider_ =
      std::make_shared<ED25519ProviderMock>();
  std::shared_ptr<SteadyClockMock> clock_ = std::make_shared<SteadyClockMock>();
  std::shared_ptr<BlockTreeMock> tree_ = std::make_shared<BlockTreeMock>();

  boost::asio::io_context io_context_;

  std::shared_ptr<VotingRoundImpl> voting_round_;
};

size_t total_weight = 0;

ACTION_P(Push, vote_weight) {
  total_weight += vote_weight;
  return VoteTracker::PushResult::SUCCESS;
}

TEST_F(VotingRoundTest, EstimateIsValid) {
  // Alice votes
  SignedPrevote alice_vote =
      preparePrevote(kAlice, kAliceSignature, Prevote{10, "FC"_H});

  EXPECT_CALL(*vote_tracker_, push(alice_vote, kAliceWeight))
      .WillOnce(Push(kAliceWeight));

  VoteWeight v_alice = preparePrevoteWeight(kAlice);

  EXPECT_CALL(*vote_graph_, insert(alice_vote.message, v_alice))
      .WillOnce(Return(outcome::success()));
  EXPECT_CALL(*vote_tracker_, prevoteWeight()).WillOnce(Return(total_weight));

  voting_round_->onPrevote(alice_vote);

  // Bob votes
  SignedPrevote bob_vote =
      preparePrevote(kBob, kBobSignature, Prevote{10, "ED"_H});

  EXPECT_CALL(*vote_tracker_, push(bob_vote, kBobWeight))
      .WillOnce(Push(kBobWeight));

  VoteWeight v_bob = preparePrevoteWeight(kBob);

  EXPECT_CALL(*vote_graph_, insert(bob_vote.message, v_bob))
      .WillOnce(Return(outcome::success()));
  EXPECT_CALL(*vote_tracker_, prevoteWeight()).WillOnce(Return(total_weight));

  voting_round_->onPrevote(bob_vote);

  // Eve votes
  SignedPrevote eve_vote =
      preparePrevote(kEve, kEveSignature, Prevote{7, "F"_H});

  EXPECT_CALL(*vote_tracker_, push(eve_vote, kEveWeight))
      .WillOnce(Push(kEveWeight));

  VoteWeight v_eve = preparePrevoteWeight(kEve);

  EXPECT_CALL(*vote_graph_, insert(eve_vote.message, v_eve))
      .WillOnce(Return(outcome::success()));
  EXPECT_CALL(*vote_tracker_, prevoteWeight()).WillOnce(Return(total_weight));

  voting_round_->onPrevote(eve_vote);
}
