/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/vote_tracker_impl.hpp"

#include <gtest/gtest.h>
#include "common/visitor.hpp"
#include "consensus/grandpa/structs.hpp"
#include "testutil/literals.hpp"

using namespace kagome::consensus::grandpa;
using kagome::common::Hash256;

class VoteTrackerTest : public testing::Test {
 public:
  using PushResult = VoteTracker::PushResult;
  using Weight = size_t;

  VoteTrackerImpl tracker;

  std::vector<Id> ids = {Id{"01"_hash256}, Id{"02"_hash256}, Id{"03"_hash256}};
  std::vector<Weight> weights = {101, 102, 103};
  std::vector<Hash256> hashes = {"1"_hash256, "2"_hash256, "3"_hash256};

  /**
   * Convenience method that creates a signed message with a minimum of data
   * required for testing
   * @tparam M underlying message type
   * @param id peerid of he voter
   * @param hash hash of the block voted for
   * @return the message created
   */
  SignedMessage createMessage(const Id &id, const Hash256 &hash) {
    SignedMessage m;
    m.id = id;
    Prevote msg;
    msg.hash = hash;
    m.message = std::move(msg);
    return m;
  }

  // tuples (message, weight, expected push result)
  const std::vector<std::tuple<SignedMessage, Weight, PushResult>> messages{
      // First vote of each voter is accepting
      {createMessage(ids[0], hashes[0]), weights[0], PushResult::SUCCESS},
      // Repeat known vote of voter is duplicate
      {createMessage(ids[0], hashes[0]), weights[0], PushResult::DUPLICATED},
      // Another one vote of each known voter is equivocation
      {createMessage(ids[0], hashes[1]), weights[0], PushResult::EQUIVOCATED},
      {createMessage(ids[0], hashes[2]), weights[0], PushResult::EQUIVOCATED},
      {createMessage(ids[0], hashes[3]), weights[0], PushResult::EQUIVOCATED},
      // Repeat known vote of equivocator is equivocation anyway
      {createMessage(ids[0], hashes[0]), weights[0], PushResult::EQUIVOCATED},

      {createMessage(ids[1], hashes[2]), weights[1], PushResult::SUCCESS},
      {createMessage(ids[1], hashes[1]), weights[1], PushResult::EQUIVOCATED}};
};

/**
 * @given an empty vote tracker
 * @when pushing a vote to it
 * @then the result matches expectations (that are made according to push method
 * description)
 */
TEST_F(VoteTrackerTest, Push) {
  for (auto &[m, w, r] : messages) {
    ASSERT_EQ(tracker.push(m, w), r);
  }
}

/**
 * @given an empty vote tracker
 * @when pushing votes to it
 * @then the total weight is the weight of all non-duplicate votes
 */
TEST_F(VoteTrackerTest, Weight) {
  Weight expected_weight = 0;
  for (auto &[vote, weight, expected_result] : messages) {
    auto result = tracker.push(vote, weight);
    if (result == VoteTrackerTest::PushResult::SUCCESS) {
      expected_weight += weight;
    }
  }
  ASSERT_EQ(tracker.getTotalWeight(), expected_weight);
}

/**
 * @given an empty vote tracker
 * @when pushing votes to it
 * @then the message set contains vote of each honest voters and two first votes
 * of each equivocators
 */
TEST_F(VoteTrackerTest, GetMessages) {
  std::list<SignedMessage> expected;
  std::set<Id> equivocators;
  for (auto &[m, w, r] : messages) {
    tracker.push(m, w);
    if (r == VoteTrackerTest::PushResult::SUCCESS) {
      expected.push_back(m);
    } else if (r == VoteTrackerTest::PushResult::EQUIVOCATED) {
      if (equivocators.emplace(m.id).second) {
        expected.push_back(m);
      }
    }
  }
  auto messages = tracker.getMessages();

  for (auto &m : expected) {
    ASSERT_TRUE(
        std::find_if(
            messages.begin(),
            messages.end(),
            [&m](auto &v) {
              return kagome::visit_in_place(
                  v,
                  [&](const SignedMessage &voting_message) {
                    return m.id == voting_message.id
                           && m.getBlockHash() == voting_message.getBlockHash();
                  },
                  [&](const EquivocatorySignedMessage
                          &equivocatory_voting_message) {
                    const auto &first_id = equivocatory_voting_message.first.id;
                    const auto &first_block_hash =
                        equivocatory_voting_message.first.getBlockHash();

                    const auto &second_id =
                        equivocatory_voting_message.second.id;
                    const auto &second_block_hash =
                        equivocatory_voting_message.second.getBlockHash();

                    return (m.id == first_id
                            && m.getBlockHash() == first_block_hash)
                           || (m.id == second_id
                               && m.getBlockHash() == second_block_hash);
                  });
            })
        != messages.end());
  }
}

/**
 * Equivocating scenario
 */
TEST_F(VoteTrackerTest, Equivocated) {
  /// @given an empty vote tracker

  /// @when pushing first vote of voter
  /// @then vote is accepting successful
  ASSERT_EQ(tracker.push(createMessage(ids[0], hashes[0]), weights[0]),
            PushResult::SUCCESS);

  /// @when pushing another one vote of known voter
  /// @then vote is accepting as equivocation, and does not change total weight
  ASSERT_EQ(tracker.push(createMessage(ids[0], hashes[1]), weights[0]),
            PushResult::EQUIVOCATED);

  /// @when pushing anyone vote of known equivocator
  /// @then vote is not accepting, and does not change state

  // Repeat known vote of equivocator is equivocation anyway
  ASSERT_EQ(tracker.push(createMessage(ids[0], hashes[2]), weights[0]),
            PushResult::EQUIVOCATED);

  // Weight of equivocator is taken once
  ASSERT_EQ(tracker.getTotalWeight(), weights[0]);
}
