/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "core/consensus/grandpa/vote_graph/fixture.hpp"

TEST_F(VoteGraphFixture, GhostIntroduceBranch) {
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

    expect_getAncestry(
        "FB"_H, "FA"_H, "F"_H, "E"_H, "D"_H, "C"_H, "B"_H, "A"_H);
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{10, "FC"_H}, "5"_W));

    AssertGraphCorrect(
        *graph,
        R"({
  "entries": {
    "FC": {
      "number": 10,
      "ancestors": [
        "FB",
        "FA",
        "F",
        "E",
        "D",
        "C",
        "B",
        "A",
        "genesis"
      ],
      "descendents": [],
      "cumulative_vote": 5
    },
    "genesis": {
      "number": 1,
      "ancestors": [],
      "descendents": [
        "FC"
      ],
      "cumulative_vote": 5
    }
  },
  "heads": [
    "FC"
  ],
  "base": "genesis",
  "base_number": 1
})");

    expect_getAncestry(
        "EC"_H, "EB"_H, "EA"_H, "E"_H, "D"_H, "C"_H, "B"_H, "A"_H);
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{10, "ED"_H}, "7"_W));

    AssertGraphCorrect(
        *graph,
        R"({
  "entries": {
    "ED": {
      "number": 10,
      "ancestors": [
        "EC",
        "EB",
        "EA",
        "E",
        "D",
        "C",
        "B",
        "A",
        "genesis"
      ],
      "descendents": [],
      "cumulative_vote": 7
    },
    "FC": {
      "number": 10,
      "ancestors": [
        "FB",
        "FA",
        "F",
        "E",
        "D",
        "C",
        "B",
        "A",
        "genesis"
      ],
      "descendents": [],
      "cumulative_vote": 5
    },
    "genesis": {
      "number": 1,
      "ancestors": [],
      "descendents": [
        "FC",
        "ED"
      ],
      "cumulative_vote": 12
    }
  },
  "heads": [
    "ED",
    "FC"
  ],
  "base": "genesis",
  "base_number": 1
})");
  }

  {
    auto ghostOpt =
        graph->findGhost(boost::none, [](auto &&x) { return x >= "10"_W; });
    ASSERT_TRUE(ghostOpt);
    ASSERT_EQ(*ghostOpt, BlockInfo(6, "E"_H));
  }

  {  // introduce branch in the middle
    // do not expect that insert is calling getAncestry
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{6, "E"_H}, "3"_W));

    AssertGraphCorrect(
        *graph,
        R"({
  "entries": {
    "genesis": {
      "number": 1,
      "ancestors": [],
      "descendents": [
        "E"
      ],
      "cumulative_vote": 15
    },
    "ED": {
      "number": 10,
      "ancestors": [
        "EC",
        "EB",
        "EA",
        "E"
      ],
      "descendents": [],
      "cumulative_vote": 7
    },
    "FC": {
      "number": 10,
      "ancestors": [
        "FB",
        "FA",
        "F",
        "E"
      ],
      "descendents": [],
      "cumulative_vote": 5
    },
    "E": {
      "number": 6,
      "ancestors": [
        "D",
        "C",
        "B",
        "A",
        "genesis"
      ],
      "descendents": [
        "ED",
        "FC"
      ],
      "cumulative_vote": 15
    }
  },
  "heads": [
    "ED",
    "FC"
  ],
  "base": "genesis",
  "base_number": 1
})");
  }

  auto check = [&](const boost::optional<BlockInfo> &block) {
    auto ghostOpt =
        graph->findGhost(block, [](auto &&x) { return x >= "10"_W; });
    ASSERT_TRUE(ghostOpt);
    BlockInfo EXPECTED(6, "E"_H);
    ASSERT_EQ(*ghostOpt, EXPECTED);
  };

  check(boost::none);
  check(BlockInfo(4, "C"_H));
  check(BlockInfo(6, "E"_H));
}
