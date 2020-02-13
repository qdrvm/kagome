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

template <typename Message>
class VoteTrackerTest : public testing::Test {
 public:
  using PushResult = typename VoteTracker<Message>::PushResult;

  VoteTrackerImpl<Message> tracker;
  std::vector<Id> ids = {{"01"_hash256}, {"02"_hash256}, {"03"_hash256}};
  std::vector<Hash256> hashes = {
      "010203"_hash256, "040506"_hash256, "070809"_hash256};

  /**
   * Convenience method that creates a signed message with a minimum of data
   * required for testing
   * @tparam M underlying message type
   * @param id peerid of he voter
   * @param hash hash of the block voted for
   * @return the message created
   */
  SignedMessage<Message> createMessage(const Id &id, const Hash256 &hash) {
    SignedMessage<Message> m;
    m.id = id;
    m.message.block_hash = hash;
    return m;
  }

  // tuples (message, weight, expected push result)
  std::vector<std::tuple<SignedMessage<Message>, size_t, PushResult>>
      test_messages{
          {createMessage(ids[0], hashes[0]), 3, PushResult::SUCCESS},
          {createMessage(ids[0], hashes[0]), 7, PushResult::DUPLICATED},
          {createMessage(ids[0], hashes[1]), 2, PushResult::EQUIVOCATED},
          {createMessage(ids[0], hashes[2]), 8, PushResult::DUPLICATED},
          {createMessage(ids[1], hashes[2]), 3, PushResult::SUCCESS},
          {createMessage(ids[1], hashes[1]), 1, PushResult::EQUIVOCATED}};
};

TYPED_TEST_CASE_P(VoteTrackerTest);

/**
 * @given an empty vote tracker
 * @when pushing a vote to it
 * @then the result matches expectations (that are made according to push method
 * description)
 */
TYPED_TEST_P(VoteTrackerTest, Push) {
  for (auto &[m, w, r] : this->test_messages) {
    ASSERT_EQ(this->tracker.push(m, w), r);
  }
}

/**
 * @given an empty vote tracker
 * @when pushing votes to it
 * @then the total weight is the weight of all non-duplicate votes
 */
TYPED_TEST_P(VoteTrackerTest, Weight) {
  size_t expected_weight = 0;
  for (auto &[m, w, r] : this->test_messages) {
    this->tracker.push(m, w);
    if (r != VoteTrackerTest<TypeParam>::PushResult::DUPLICATED) {
      expected_weight += w;
    }
  }
  ASSERT_EQ(this->tracker.getTotalWeight(), expected_weight);
}

/**
 * @given an empty vote tracker
 * @when pushing votes to it
 * @then the message set contains all non-duplicate messaged
 */
TYPED_TEST_P(VoteTrackerTest, GetMessages) {
  std::list<SignedMessage<TypeParam>> expected;
  for (auto &[m, w, r] : this->test_messages) {
    this->tracker.push(m, w);
    if (r != VoteTrackerTest<TypeParam>::PushResult::DUPLICATED) {
      expected.push_back(m);
    }
  }
  auto messages = this->tracker.getMessages();

  for (auto &m : expected) {
    ASSERT_TRUE(
        std::find_if(
            messages.begin(),
            messages.end(),
            [&m](auto &v) {
              return kagome::visit_in_place(
                  v,
                  [&](const typename decltype(
                      this->tracker)::VotingMessage &voting_message) {
                    return m.id == voting_message.id
                           && m.message.block_hash
                                  == voting_message.message.block_hash;
                  },
                  [&](const typename decltype(
                      this->tracker)::EquivocatoryVotingMessage
                          &equivocatory_voting_message) {
                    auto first_id = equivocatory_voting_message.first.id;
                    auto first_block_hash =
                        equivocatory_voting_message.first.message.block_hash;

                    auto second_id = equivocatory_voting_message.second.id;
                    auto second_block_hash =
                        equivocatory_voting_message.second.message.block_hash;
                    return (m.id == first_id
                            && m.message.block_hash == first_block_hash)
                           || (m.id == second_id
                               && m.message.block_hash == second_block_hash);
                  });
            })
        != messages.end());
  }
}

/**
 * @given an empty vote tracker
 * @when pushin three votes for different blocks from one peer
 * @then the first one is SUCCESS and accepted, the second one is EQUIVOCATED
 * and accepted, too, the third one is a DUPLICATE and is not accepted(e. g.
 * does not affect total weight)
 */
TYPED_TEST_P(VoteTrackerTest, Equivocated) {
  using PushResult = typename VoteTrackerTest<TypeParam>::PushResult;
  ASSERT_EQ(
      this->tracker.push(this->createMessage(this->ids[0], this->hashes[0]), 3),
      PushResult::SUCCESS);
  ASSERT_EQ(
      this->tracker.push(this->createMessage(this->ids[0], this->hashes[1]), 1),
      PushResult::EQUIVOCATED);
  ASSERT_EQ(
      this->tracker.push(this->createMessage(this->ids[0], this->hashes[2]), 5),
      PushResult::DUPLICATED);
  ASSERT_EQ(this->tracker.getTotalWeight(), 4);
}

REGISTER_TYPED_TEST_CASE_P(
    VoteTrackerTest, Push, Weight, GetMessages, Equivocated);

using TestTypes = testing::Types<Prevote, Precommit>;
INSTANTIATE_TYPED_TEST_CASE_P(VoteTrackerTests, VoteTrackerTest, TestTypes);
