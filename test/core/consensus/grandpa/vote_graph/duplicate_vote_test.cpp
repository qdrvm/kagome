/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "core/consensus/grandpa/vote_graph/fixture.hpp"

/**
 * @given graph with added votes
 * @when add vote voter which vote has already added before
 * @then vote applied, sum weight is recalculated without duplication
 */
TEST_F(VoteGraphFixture, DuplicateVote) {
  // GIVEN

  BlockInfo base{0, GENESIS_HASH};
  auto voter = "w10_a"_ID;
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

  expect_getAncestry(
      GENESIS_HASH, "C"_H, vec("C"_H, "B"_H, "A"_H, GENESIS_HASH));
  EXPECT_OUTCOME_TRUE_1(graph->insert(vt, {3, "C"_H}, voter));

  AssertGraphCorrect(*graph, R"(
        {
          "entries": {
            "genesis": {
              "number": 0,
              "ancestors": [],
              "descendants": [
                "C"
              ],
              "cumulative_vote": 10
            },
            "C": {
              "number": 3,
              "ancestors": [
                "B",
                "A",
                "genesis"
              ],
              "descendants": [],
              "cumulative_vote": 10
            }
          },
          "heads": [
            "C"
          ],
          "base": "genesis",
          "base_number": 0
        }
  )");

  // THEN.1

  expect_getAncestry(
      GENESIS_HASH, "D"_H, vec("D"_H, "C"_H, "B"_H, "A"_H, GENESIS_HASH));
  EXPECT_OUTCOME_TRUE_1(graph->insert(vt, {4, "D"_H}, voter));

  // WHEN.1

  AssertGraphCorrect(*graph, R"(
        {
          "entries": {
            "genesis": {
              "number": 0,
              "ancestors": [],
              "descendants": [
                "C"
              ],
              "cumulative_vote": 10
            },
            "C": {
              "number": 3,
              "ancestors": [
                "B",
                "A",
                "genesis"
              ],
              "descendants": [
                "D"
              ],
              "cumulative_vote": 10
            },
            "D": {
              "number": 4,
              "ancestors": [
                "C"
              ],
              "descendants": [],
              "cumulative_vote": 10
            }
          },
          "heads": [
            "D"
          ],
          "base": "genesis",
          "base_number": 0
        }
  )");

  // WHEN.2

  EXPECT_OUTCOME_TRUE_1(graph->insert(vt, {3, "C"_H}, voter));

  // THEN.2

  AssertGraphCorrect(*graph, R"(
        {
          "entries": {
            "genesis": {
              "number": 0,
              "ancestors": [],
              "descendants": [
                "C"
              ],
              "cumulative_vote": 10
            },
            "C": {
              "number": 3,
              "ancestors": [
                "B",
                "A",
                "genesis"
              ],
              "descendants": [
                "D"
              ],
              "cumulative_vote": 10
            },
            "D": {
              "number": 4,
              "ancestors": [
                "C"
              ],
              "descendants": [],
              "cumulative_vote": 10
            }
          },
          "heads": [
            "D"
          ],
          "base": "genesis",
          "base_number": 0
        }
  )");
}
