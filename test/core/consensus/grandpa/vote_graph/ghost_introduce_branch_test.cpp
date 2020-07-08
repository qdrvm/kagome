/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "core/consensus/grandpa/vote_graph/fixture.hpp"

TEST_F(VoteGraphFixture, GhostIntroduceBranch) {
  BlockInfo base{1, GENESIS_HASH};
  graph = std::make_shared<VoteGraphImpl>(base, chain);

  {  // insert nodes
    AssertGraphCorrect(*graph,
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
        GENESIS_HASH,
        "FC"_H,
        vec("FB"_H, "FA"_H, "F"_H, "E"_H, "D"_H, "C"_H, "B"_H, "A"_H));
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{10, "FC"_H}, "5"_W));

    AssertGraphCorrect(*graph,
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
        GENESIS_HASH,
        "ED"_H,
        vec("EC"_H, "EB"_H, "EA"_H, "E"_H, "D"_H, "C"_H, "B"_H, "A"_H));
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{10, "ED"_H}, "7"_W));

    AssertGraphCorrect(*graph,
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
        graph->findGhost(boost::none, [](auto &&x) { return x >= "5"_W; });
    ASSERT_TRUE(ghostOpt);
    ASSERT_EQ(*ghostOpt, BlockInfo(10, "ED"_H))
        << "The farthest block of blocks with enough weight should be selected";
  }

  {
    auto ghostOpt =
        graph->findGhost(boost::none, [](auto &&x) { return x >= "10"_W; });
    ASSERT_TRUE(ghostOpt);
    ASSERT_EQ(*ghostOpt, BlockInfo(6, "E"_H))
        << "A highest-weighted of blocks with enough weight should be selected";
  }

  {  // introduce branch in the middle
    // do not expect that insert is calling getAncestry
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{6, "E"_H}, "3"_W));

    AssertGraphCorrect(*graph,
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

  auto check = [&](const boost::optional<BlockInfo> &block,
                   const VoteWeight &v,
                   const boost::optional<BlockInfo> &expected,
                   std::string_view comment) {
    auto ghostOpt = graph->findGhost(block, [&v](auto &&x) { return x >= v; });
    ASSERT_TRUE(ghostOpt) << comment;
    ASSERT_EQ(*ghostOpt, *expected) << comment;
  };

  // clang-format off

  // # 1   2   3   4   5   6      7    8    9    10
  //
  //                                             +5
  //                                 - FA - FB - FC
  //                       +3      /   =5   =5   =5
  // GEN - A - B - C - D - E +--- F
  // =15  =15 =15 =15 =15 =15 \  =5
  //                           \                 +7
  //                            - EA - EB - EC - ED
  //                              =7   =7   =7   =7

  //    Reviewing block      Weight  Expecting block      Comment
  check(boost::none,         "10"_W, BlockInfo(6, "E"_H),  "From base with weight 10");
  check(BlockInfo(5, "D"_H), "10"_W, BlockInfo(6, "E"_H),  "From D with weight 10");
  check(BlockInfo(6, "E"_H), "10"_W, BlockInfo(6, "E"_H),  "From E with weight 10");
  check(BlockInfo(7, "F"_H), "10"_W, BlockInfo(6, "E"_H),  "From F with weight 10");

  check(boost::none,         "7"_W, BlockInfo(10, "ED"_H), "From base with weight 7");
  check(BlockInfo(5, "D"_H), "7"_W, BlockInfo(10, "ED"_H), "From D with weight 7");
  check(BlockInfo(6, "E"_H), "7"_W, BlockInfo(10, "ED"_H), "From E with weight 7");
  check(BlockInfo(7, "F"_H), "7"_W, BlockInfo(6, "E"_H),   "From F with weight 7");

  check(boost::none,         "5"_W, BlockInfo(10, "ED"_H), "From base with weight 5");
  check(BlockInfo(5, "D"_H), "5"_W, BlockInfo(10, "ED"_H), "From D with weight 5");
  check(BlockInfo(6, "E"_H), "5"_W, BlockInfo(10, "ED"_H), "From E with weight 5");
  check(BlockInfo(7, "F"_H), "5"_W, BlockInfo(10, "FC"_H), "From F with weight 5");
}
