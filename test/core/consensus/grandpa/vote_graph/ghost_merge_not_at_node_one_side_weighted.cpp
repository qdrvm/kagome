/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "core/consensus/grandpa/vote_graph/fixture.hpp"

TEST_F(VoteGraphFixture, GhostMergeNotAtNodeOneSideWeighted) {
  BlockInfo base{1, GENESIS_HASH};
  graph = std::make_shared<VoteGraphImpl>(base, chain);

  {  // insert nodes
    AssertGraphCorrect(
        *graph,
        R"({
  "entries": {
    "genesis": {
      "number": 1,
      "ancestors": [],
      "descendents": [],
      "cumulative_vote": 0
    }
  },
  "heads": [
    "genesis"
  ],
  "base": "genesis",
  "base_number": 1
})");

    expect_getAncestry("A"_H);
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{3, "B"_H}, "0"_W));

    AssertGraphCorrect(
        *graph,
        R"({
  "entries": {
    "genesis": {
      "number": 1,
      "ancestors": [],
      "descendents": [
        "B"
      ],
      "cumulative_vote": 0
    },
    "B": {
      "number": 3,
      "ancestors": [
        "A",
        "genesis"
      ],
      "descendents": [],
      "cumulative_vote": 0
    }
  },
  "heads": [
    "B"
  ],
  "base": "genesis",
  "base_number": 1
})");

    expect_getAncestry("F"_H, "E"_H, "D"_H, "C"_H, "B"_H, "A"_H);
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{8, "G1"_H}, "100"_W));

    AssertGraphCorrect(
        *graph,
        R"({
  "entries": {
    "G1": {
      "number": 8,
      "ancestors": [
        "F",
        "E",
        "D",
        "C",
        "B"
      ],
      "descendents": [],
      "cumulative_vote": 100
    },
    "genesis": {
      "number": 1,
      "ancestors": [],
      "descendents": [
        "B"
      ],
      "cumulative_vote": 100
    },
    "B": {
      "number": 3,
      "ancestors": [
        "A",
        "genesis"
      ],
      "descendents": [
        "G1"
      ],
      "cumulative_vote": 100
    }
  },
  "heads": [
    "G1"
  ],
  "base": "genesis",
  "base_number": 1
})");

    expect_getAncestry("G2"_H, "F"_H, "E"_H, "D"_H, "C"_H, "B"_H, "A"_H);
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{9, "H2"_H}, "150"_W));

    AssertGraphCorrect(
        *graph,
        R"({
  "entries": {
    "H2": {
      "number": 9,
      "ancestors": [
        "G2",
        "F",
        "E",
        "D",
        "C",
        "B"
      ],
      "descendents": [],
      "cumulative_vote": 150
    },
    "genesis": {
      "number": 1,
      "ancestors": [],
      "descendents": [
        "B"
      ],
      "cumulative_vote": 250
    },
    "B": {
      "number": 3,
      "ancestors": [
        "A",
        "genesis"
      ],
      "descendents": [
        "G1",
        "H2"
      ],
      "cumulative_vote": 250
    },
    "G1": {
      "number": 8,
      "ancestors": [
        "F",
        "E",
        "D",
        "C",
        "B"
      ],
      "descendents": [],
      "cumulative_vote": 100
    }
  },
  "heads": [
    "G1",
    "H2"
  ],
  "base": "genesis",
  "base_number": 1
})");
  }

  auto check = [&](const boost::optional<BlockInfo> &block) {
    auto ghostOpt =
        graph->findGhost(block, [](auto &&x) { return x >= "250"_W; });
    ASSERT_TRUE(ghostOpt);
    BlockInfo EXPECTED(7, "F"_H);
    ASSERT_EQ(*ghostOpt, EXPECTED);
  };

  check(boost::none);
  check(BlockInfo(7, "B"_H));
  check(BlockInfo(4, "C"_H));
  check(BlockInfo(3, "B"_H));
}
