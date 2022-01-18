/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "consensus/grandpa/vote_weight.hpp"

using kagome::consensus::grandpa::VoteWeight;

class VoteWeightTest : public testing::Test {
 protected:
  void SetUp() override {
    testee = std::make_unique<VoteWeight::OneTypeVoteWeight>();
  }

  static constexpr VoteWeight::Weight w[] = {1, 10, 100};
  std::unique_ptr<VoteWeight::OneTypeVoteWeight> testee;
};

/**
 * @given clean VoteWeight
 * @when add votes of several different voters
 * @then these votes applied, sum is equal sum of weight all votes
 */
TEST_F(VoteWeightTest, AddDifferentVote) {
  // GIVEN

  // WHEN.1
  testee->set(0, w[0]);

  // THEN.1
  EXPECT_EQ(testee->sum, w[0]);
  EXPECT_EQ(std::count(testee->flags.begin(), testee->flags.end(), true), 1);

  // WHEN.2
  testee->set(2, w[2]);

  // THEN.2
  EXPECT_EQ(testee->sum, w[0] + w[2]);
  EXPECT_EQ(std::count(testee->flags.begin(), testee->flags.end(), true), 2);

  // WHEN.3
  testee->set(1, w[1]);

  // THEN.3
  EXPECT_EQ(testee->sum, w[0] + w[1] + w[2]);
  EXPECT_EQ(std::count(testee->flags.begin(), testee->flags.end(), true), 3);
}

/**
 * @given VoteWeight with added votes
 * @when add votes from voters, which already added
 * @then these votes have not applied, state did not change
 */
TEST_F(VoteWeightTest, AddExistingVote) {
  // GIVEN
  testee->set(0, w[0]);
  testee->set(1, w[1]);
  testee->set(2, w[2]);
  ASSERT_EQ(testee->sum, w[0] + w[1] + w[2]);
  ASSERT_EQ(std::count(testee->flags.begin(), testee->flags.end(), true), 3);

  // WHEN.1
  testee->set(0, w[0]);

  // THEN.1
  EXPECT_EQ(testee->sum, w[0] + w[1] + w[2]);
  EXPECT_EQ(std::count(testee->flags.begin(), testee->flags.end(), true), 3);

  // WHEN.2
  testee->set(1, w[1]);

  // WHEN.2
  EXPECT_EQ(testee->sum, w[0] + w[1] + w[2]);
  EXPECT_EQ(std::count(testee->flags.begin(), testee->flags.end(), true), 3);

  // THEN.3
  testee->set(2, w[2]);

  // WHEN.3
  EXPECT_EQ(testee->sum, w[0] + w[1] + w[2]);
  EXPECT_EQ(std::count(testee->flags.begin(), testee->flags.end(), true), 3);
}

/**
 * @given VoteWeight with added votes
 * @when remove votes from voters, which already added before
 * @then these votes removed
 */
TEST_F(VoteWeightTest, RemoveExistingVote) {
  // GIVEN
  testee->set(0, w[0]);
  testee->set(1, w[1]);
  testee->set(2, w[2]);
  ASSERT_EQ(testee->sum, w[0] + w[1] + w[2]);
  ASSERT_EQ(std::count(testee->flags.begin(), testee->flags.end(), true), 3);

  // WHEN.1
  testee->unset(1, w[1]);

  // THEN.1
  EXPECT_EQ(testee->sum, w[0] + w[2]);
  EXPECT_EQ(std::count(testee->flags.begin(), testee->flags.end(), true), 2);

  // WHEN.2
  testee->unset(0, w[0]);

  // THEN.2
  EXPECT_EQ(testee->sum, w[2]);
  EXPECT_EQ(std::count(testee->flags.begin(), testee->flags.end(), true), 1);

  // WHEN.3
  testee->unset(2, w[2]);

  // THEN.3
  EXPECT_EQ(testee->sum, 0);
  EXPECT_EQ(std::count(testee->flags.begin(), testee->flags.end(), true), 0);
}

/**
 * @given VoteWeight with added votes
 * @when remove votes from voters, which have not added before
 * @then state did not change
 */
TEST_F(VoteWeightTest, RemoveNonExistingVote) {
  // GIVEN
  testee->set(0, w[0]);
  testee->set(2, w[2]);
  ASSERT_EQ(testee->sum, w[0] + w[2]);
  ASSERT_EQ(std::count(testee->flags.begin(), testee->flags.end(), true), 2);

  // WHEN
  testee->unset(1, w[1]);

  // THEN
  EXPECT_EQ(testee->sum, w[0] + w[2]);
  EXPECT_EQ(std::count(testee->flags.begin(), testee->flags.end(), true), 2);
}
