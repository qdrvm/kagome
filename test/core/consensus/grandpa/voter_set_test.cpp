/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "consensus/grandpa/voter_set.hpp"
#include "core/consensus/grandpa/literals.hpp"
#include "testutil/outcome.hpp"

using kagome::consensus::grandpa::Id;
using kagome::consensus::grandpa::VoterSet;
using Weight = kagome::consensus::grandpa::VoterSet::Weight;

class VoterSetTest : public testing::Test {
 protected:
  void SetUp() override {
    testee = std::make_unique<VoterSet>();
  }

  std::vector<std::tuple<Id, Weight>> voters{
      {"A"_ID, 1}, {"B"_ID, 2}, {"C"_ID, 2}, {"D"_ID, 2}, {"E"_ID, 3}};

  std::unique_ptr<VoterSet> testee;
};

/**
 * @given clean VoteWeight
 * @when add votes of several different voters
 * @then these votes applied, sum is equal sum of weight all votes
 */
TEST_F(VoterSetTest, FillDifferentVoters) {
  // GIVEN

  for (auto &[voter, weight] : voters) {
    // WHEN
    auto res = testee->insert(voter, weight);

    // THEN
    EXPECT_TRUE(res.has_value());
  }
}

TEST_F(VoterSetTest, AddExistingVoters) {
  // GIVEN

  for (auto &[voter, weight] : voters) {
    ASSERT_OUTCOME_SUCCESS_TRY(testee->insert(voter, weight));
  }

  for (auto &[voter, weight] : voters) {
    // WHEN+THEN
    EXPECT_OUTCOME_ERROR(res,
                         testee->insert(voter, weight),
                         VoterSet::Error::VOTER_ALREADY_EXISTS);
  }
}

TEST_F(VoterSetTest, GetIndex) {
  // GIVEN

  for (auto &[voter, weight] : voters) {
    ASSERT_OUTCOME_SUCCESS_TRY(testee->insert(voter, weight));
  }

  for (auto &[voter, weight] : voters) {
    // WHEN+THEN.1
    ASSERT_NE(std::nullopt, testee->voterIndex(voter));
  }

  // WHEN+THEN.2
  ASSERT_EQ(testee->voterIndex("Unknown"_ID), std::nullopt);
}

TEST_F(VoterSetTest, GetWeight) {
  // GIVEN

  for (auto &[voter, weight] : voters) {
    ASSERT_OUTCOME_SUCCESS_TRY(testee->insert(voter, weight));
  }

  size_t index = 0;
  for (auto &[voter, weight] : voters) {
    {
      // WHEN+THEN.1
      auto actual_weight_opt = testee->voterWeight(voter);
      ASSERT_TRUE(actual_weight_opt.has_value());
      auto &actual_weight = actual_weight_opt.value();
      EXPECT_EQ(weight, actual_weight);
    }
    {
      // WHEN+THEN.2
      EXPECT_OUTCOME_SUCCESS(actual_weight_res, testee->voterWeight(index));
      auto &actual_weight = actual_weight_res.value();
      EXPECT_EQ(weight, actual_weight);
    }
    ++index;
  }

  {
    // WHEN+THEN.3
    ASSERT_EQ(std::nullopt, testee->voterWeight("Unknown"_ID));
  }
  {
    // WHEN+THEN.4
    EXPECT_OUTCOME_ERROR(res,
                         testee->voterWeight(voters.size()),
                         VoterSet::Error::INDEX_OUTBOUND);
  }
}

TEST_F(VoterSetTest, GetVoter) {
  // GIVEN

  for (auto &[voter, weight] : voters) {
    ASSERT_OUTCOME_SUCCESS_TRY(testee->insert(voter, weight));
  }

  for (size_t index = 0; index < voters.size(); ++index) {
    // WHEN+THEN.1
    EXPECT_OUTCOME_SUCCESS(voter, testee->voterId(index));
  }

  {
    // WHEN+THEN.2
    EXPECT_OUTCOME_ERROR(
        res, testee->voterId(voters.size()), VoterSet::Error::INDEX_OUTBOUND);
  }
}

TEST_F(VoterSetTest, GetIndexAndWeight) {
  // GIVEN

  for (auto &[voter, weight] : voters) {
    ASSERT_OUTCOME_SUCCESS_TRY(testee->insert(voter, weight));
  }

  size_t index = 0;
  for (auto &[voter, weight] : voters) {
    // WHEN+THEN.1
    auto res = testee->indexAndWeight(voter);
    ASSERT_NE(res, std::nullopt);
    auto [actual_index, actual_weight] = res.value();
    EXPECT_TRUE(actual_index == index);
    EXPECT_TRUE(actual_weight == weight);
    ++index;
  }

  {
    // WHEN+THEN.2
    ASSERT_EQ(std::nullopt, testee->indexAndWeight("Unknown"_ID));
  }
}
