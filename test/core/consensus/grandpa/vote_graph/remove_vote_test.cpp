/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "core/consensus/grandpa/vote_graph/fixture.hpp"

/**
 * @given graph with added votes
 * @when remove existing vote of one of voter
 * @then vote retracted, sum weight is recalculated
 */
TEST_F(VoteGraphFixture, RetractVote) {

  // GIVEN

  BlockInfo base{0, GENESIS_HASH};
  graph = std::make_shared<VoteGraphImpl>(base, voter_set, chain);

  AssertGraphCorrect(*graph, R"(
         {
           "entries": {
             "genesis": {
               "number": 0,
               "ancestors": [],
               "descendants": [],
               "cumulative_vote": 0
             }
           },
           "heads": [
             "genesis"
           ],
           "base": "genesis",
           "base_number": 0
         }
  )");

  expect_getAncestry(GENESIS_HASH, "A"_H, vec("A"_H, GENESIS_HASH));
  EXPECT_OUTCOME_TRUE_1(graph->insert({1, "A"_H}, "w1_a"_ID));

  AssertGraphCorrect(*graph, R"(
        {
          "entries": {
            "genesis": {
              "number": 0,
              "ancestors": [],
              "descendants": [
                "A"
              ],
              "cumulative_vote": 1
            },
            "A": {
              "number": 1,
              "ancestors": [
                "genesis"
              ],
              "descendants": [],
              "cumulative_vote": 1
            }
          },
          "heads": [
            "A"
          ],
          "base": "genesis",
          "base_number": 0
        }
  )");

  expect_getAncestry(GENESIS_HASH, "B"_H, vec("B"_H, "A"_H, GENESIS_HASH));
  EXPECT_OUTCOME_TRUE_1(graph->insert({2, "B"_H}, "w3_a"_ID));

  AssertGraphCorrect(*graph, R"(
        {
          "entries": {
            "genesis": {
              "number": 0,
              "ancestors": [],
              "descendants": [
                "A"
              ],
              "cumulative_vote": 4
            },
            "A": {
              "number": 1,
              "ancestors": [
                "genesis"
              ],
              "descendants": [
                "B"
              ],
              "cumulative_vote": 4
            },
            "B": {
              "number": 2,
              "ancestors": [
                "A"
              ],
              "descendants": [],
              "cumulative_vote": 3
            }
          },
          "heads": [
            "B"
          ],
          "base": "genesis",
          "base_number": 0
        }
  )");

  expect_getAncestry(
      GENESIS_HASH, "C"_H, vec("C"_H, "B"_H, "A"_H, GENESIS_HASH));
  EXPECT_OUTCOME_TRUE_1(graph->insert({3, "C"_H}, "w7_a"_ID));

  AssertGraphCorrect(*graph, R"(
        {
          "entries": {
            "genesis": {
              "number": 0,
              "ancestors": [],
              "descendants": [
                "A"
              ],
              "cumulative_vote": 11
            },
            "A": {
              "number": 1,
              "ancestors": [
                "genesis"
              ],
              "descendants": [
                "B"
              ],
              "cumulative_vote": 11
            },
            "B": {
              "number": 2,
              "ancestors": [
                "A"
              ],
              "descendants": [
                "C"
              ],
              "cumulative_vote": 10
            },
            "C": {
              "number": 3,
              "ancestors": [
                "B"
              ],
              "descendants": [],
              "cumulative_vote": 7
            }
          },
          "heads": [
            "C"
          ],
          "base": "genesis",
          "base_number": 0
        }
  )");

  // WHEN

  graph->remove("w3_a"_ID);

  // THEN

  AssertGraphCorrect(*graph, R"(
        {
          "entries": {
            "genesis": {
              "number": 0,
              "ancestors": [],
              "descendants": [
                "A"
              ],
              "cumulative_vote": 8
            },
            "A": {
              "number": 1,
              "ancestors": [
                "genesis"
              ],
              "descendants": [
                "B"
              ],
              "cumulative_vote": 8
            },
            "B": {
              "number": 2,
              "ancestors": [
                "A"
              ],
              "descendants": [
                "C"
              ],
              "cumulative_vote": 7
            },
            "C": {
              "number": 3,
              "ancestors": [
                "B"
              ],
              "descendants": [],
              "cumulative_vote": 7
            }
          },
          "heads": [
            "C"
          ],
          "base": "genesis",
          "base_number": 0
        }
  )");
}
