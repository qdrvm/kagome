/**
* Copyright Soramitsu Co., Ltd. All Rights Reserved.
* SPDX-License-Identifier: Apache-2.0
*/

#include "core/consensus/grandpa/vote_graph/fixture.hpp"

#include "consensus/grandpa/vote_graph/vote_graph_error.hpp"

using kagome::consensus::grandpa::VoteGraphError;

/**
 * @given vote graph with base at block B with number 2
 * @when try to insert block A with number 2
 * @then error RECEIVED_BLOCK_LESS_THAN_BASE is received and graph is not changed
 */
TEST_F(VoteGraphFixture, InsertBlockLessThanBaseTest) {
 // GIVEN

 BlockInfo base{2, "B"_H};
 auto voter = "w10_a"_ID;
 graph = std::make_shared<VoteGraphImpl>(base, voter_set, chain);

 AssertGraphCorrect(*graph, R"(
        {
          "entries": {
            "B": {
              "number": 2,
              "ancestors": [],
              "descendants": [],
              "cumulative_vote": 0
            }
          },
          "heads": [
            "B"
          ],
          "base": "B",
          "base_number": 2
        }
 )");

 // WHEN

 ASSERT_OUTCOME_ERROR(graph->insert(vt, {1, "A"_H}, voter), VoteGraphError::RECEIVED_BLOCK_LESS_THAN_BASE);

 // THEN

 AssertGraphCorrect(*graph, R"(
{
          "entries": {
            "B": {
              "number": 2,
              "ancestors": [],
              "descendants": [],
              "cumulative_vote": 0
            }
          },
          "heads": [
            "B"
          ],
          "base": "B",
          "base_number": 2
        }
 )");
}