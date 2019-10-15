/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/vote_tracker_impl.hpp"
#include "consensus/grandpa/structs.hpp"

#include <gtest/gtest.h>
#include "testutil/literals.hpp"

using namespace kagome::consensus::grandpa;
using kagome::common::Hash256;
using PushResult = VoteTracker<Prevote>::PushResult;

class VoteTrackerTest
    : public testing::TestWithParam<
          std::vector<std::tuple<SignedMessage<Prevote>, size_t, PushResult>>> {
 public:
  VoteTrackerImpl<Prevote> tracker;
};

/**
 * Convenience method that creates a signed message with a minimum of data
 * required for testing
 * @tparam M underlying message type
 * @param id peerid of he voter
 * @param hash hash of the block voted for
 * @return the message created
 */
template <typename M>
SignedMessage<M> createMessage(const Id &id, const Hash256 &hash) {
  SignedMessage<M> m;
  m.id = id;
  m.message.hash = hash;
  return m;
}

std::vector<Id> ids = {"01"_hash256, "02"_hash256, "03"_hash256};
std::vector<Hash256> hashes = {
    "010203"_hash256, "040506"_hash256, "070809"_hash256};

// tuples (message, weight, expected push result)
std::vector<std::tuple<SignedMessage<Prevote>, size_t, PushResult>>
    test_prevotes{
        {createMessage<Prevote>(ids[0], hashes[0]), 3, PushResult::SUCCESS},
        {createMessage<Prevote>(ids[0], hashes[0]), 7, PushResult::DUPLICATED},
        {createMessage<Prevote>(ids[0], hashes[1]), 2, PushResult::EQUIVOCATED},
        {createMessage<Prevote>(ids[0], hashes[2]), 8, PushResult::DUPLICATED},
        {createMessage<Prevote>(ids[1], hashes[2]), 3, PushResult::SUCCESS},
        {createMessage<Prevote>(ids[1], hashes[1]),
         1,
         PushResult::EQUIVOCATED}};

/**
 * @given an empty vote tracker
 * @when pushing a vote to it
 * @then the result matches expectations (that are made according to push method
 * description)
 */
TEST_P(VoteTrackerTest, Push) {
  for (auto &[m, w, r] : GetParam()) {
    ASSERT_EQ(tracker.push(m, w), r);
  }
}

/**
 * @given an empty vote tracker
 * @when pushing votes to it
 * @then the total weight is the weight of all non-duplicate votes
 */
TEST_P(VoteTrackerTest, Weight) {
  size_t expected_weight = 0;
  for (auto &[m, w, r] : GetParam()) {
    tracker.push(m, w);
    if (r != PushResult::DUPLICATED) {
      expected_weight += w;
    }
  }
  ASSERT_EQ(tracker.getTotalWeight(), expected_weight);
}

/**
 * @given an empty vote tracker
 * @when pushing votes to it
 * @then the message set contains all non-duplicate messaged
 */
TEST_P(VoteTrackerTest, GetMessages) {
  std::list<SignedMessage<Prevote>> expected;
  for (auto &[m, w, r] : GetParam()) {
    tracker.push(m, w);
    if (r != PushResult::DUPLICATED) {
      expected.push_back(m);
    }
  }
  auto messages = tracker.getMessages();
  ASSERT_EQ(messages.size(), expected.size());
  for (auto &m : expected) {
    ASSERT_TRUE(std::find_if(messages.begin(),
                             messages.end(),
                             [&m](auto &v) {
                               return m.id == v.id
                                      && m.message.hash == v.message.hash;
                             })
                != messages.end());
  }
}

INSTANTIATE_TEST_CASE_P(VoteTrackerTests,
                        VoteTrackerTest,
                        testing::ValuesIn({test_prevotes}));

/**
 * @given an empty vote tracker
 * @when pushin three votes for different blocks from one peer
 * @then the first one is SUCCESS and accepted, the second one is EQUIVOCATED
 * and accepted, too, the third one is a DUPLICATE and is not accepted(e. g.
 * does not affect total weight)
 */
TEST_F(VoteTrackerTest, Equivocated) {
  ASSERT_EQ(tracker.push(createMessage<Prevote>(ids[0], hashes[0]), 3),
            PushResult::SUCCESS);
  ASSERT_EQ(tracker.push(createMessage<Prevote>(ids[0], hashes[1]), 1),
            PushResult::EQUIVOCATED);
  ASSERT_EQ(tracker.push(createMessage<Prevote>(ids[0], hashes[2]), 5),
            PushResult::DUPLICATED);
  ASSERT_EQ(tracker.getTotalWeight(), 4);
}
