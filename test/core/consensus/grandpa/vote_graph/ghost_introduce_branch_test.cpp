/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "core/consensus/grandpa/vote_graph/fixture.hpp"

TEST_F(VoteGraphFixture, GhostIntroduceBranch) {
  BlockInfo base{0, GENESIS_HASH};
  graph = std::make_shared<VoteGraphImpl>(base, chain);

  {  // insert nodes
    AssertGraphCorrect(*graph,
                       R"({
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
})");

    expect_getAncestry(GENESIS_HASH,
                       "FC"_H,
                       vec("FC"_H,
                           "FB"_H,
                           "FA"_H,
                           "F"_H,
                           "E"_H,
                           "D"_H,
                           "C"_H,
                           "B"_H,
                           "A"_H,
                           GENESIS_HASH));
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{9, "FC"_H}, 5_W));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "FC": {
      "number": 9,
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
      "descendants": [],
      "cumulative_vote": 5
    },
    "genesis": {
      "number": 0,
      "ancestors": [],
      "descendants": [
        "FC"
      ],
      "cumulative_vote": 5
    }
  },
  "heads": [
    "FC"
  ],
  "base": "genesis",
  "base_number": 0
})");

    expect_getAncestry(GENESIS_HASH,
                       "ED"_H,
                       vec("ED"_H,
                           "EC"_H,
                           "EB"_H,
                           "EA"_H,
                           "E"_H,
                           "D"_H,
                           "C"_H,
                           "B"_H,
                           "A"_H,
                           GENESIS_HASH));
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{9, "ED"_H}, 7_W));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "ED": {
      "number": 9,
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
      "descendants": [],
      "cumulative_vote": 7
    },
    "FC": {
      "number": 9,
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
      "descendants": [],
      "cumulative_vote": 5
    },
    "genesis": {
      "number": 0,
      "ancestors": [],
      "descendants": [
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
  "base_number": 0
})");
  }

  {
    auto ghostOpt = graph->findGhost(
        boost::none,
        [](auto &&x) { return x.prevotes_sum >= (5_W).prevotes_sum; },
        comparator);
    ASSERT_TRUE(ghostOpt);
    ASSERT_EQ(*ghostOpt, BlockInfo(9, "ED"_H))
        << "The best block of blocks with enough weight should be selected";
  }

  {
    auto ghostOpt = graph->findGhost(
        boost::none,
        [](auto &&x) { return x.prevotes_sum >= (10_W).prevotes_sum; },
        comparator);
    ASSERT_TRUE(ghostOpt);
    ASSERT_EQ(*ghostOpt, BlockInfo(5, "E"_H))
        << "A highest-weighted of blocks with enough weight should be selected";
  }

  {  // introduce branch in the middle
    // do not expect that insert is calling getAncestry
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{5, "E"_H}, 3_W));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "genesis": {
      "number": 0,
      "ancestors": [],
      "descendants": [
        "E"
      ],
      "cumulative_vote": 15
    },
    "ED": {
      "number": 9,
      "ancestors": [
        "EC",
        "EB",
        "EA",
        "E"
      ],
      "descendants": [],
      "cumulative_vote": 7
    },
    "FC": {
      "number": 9,
      "ancestors": [
        "FB",
        "FA",
        "F",
        "E"
      ],
      "descendants": [],
      "cumulative_vote": 5
    },
    "E": {
      "number": 5,
      "ancestors": [
        "D",
        "C",
        "B",
        "A",
        "genesis"
      ],
      "descendants": [
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
  "base_number": 0
})");
  }

  auto check = [&](const boost::optional<BlockInfo> &block,
                   const VoteWeight &v,
                   const boost::optional<BlockInfo> &expected,
                   std::string_view comment) {
    auto ghostOpt = graph->findGhost(
        block,
        [&v](auto &&x) { return x.prevotes_sum >= v.prevotes_sum; },
        comparator);
    ASSERT_TRUE(ghostOpt) << comment;
    ASSERT_EQ(*ghostOpt, *expected) << comment;
  };

  // clang-format off

  // # 0   1   2   3   4   5      6    7    8     9
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
  check(boost::none,         10_W, BlockInfo(5, "E"_H),  "From base with weight 10");
  check(BlockInfo(4, "D"_H), 10_W, BlockInfo(5, "E"_H),  "From D with weight 10");
  check(BlockInfo(5, "E"_H), 10_W, BlockInfo(5, "E"_H),  "From E with weight 10");
  check(BlockInfo(6, "F"_H), 10_W, BlockInfo(5, "E"_H),  "From F with weight 10");

  check(boost::none,         7_W, BlockInfo(9, "ED"_H), "From base with weight 7");
  check(BlockInfo(4, "D"_H), 7_W, BlockInfo(9, "ED"_H), "From D with weight 7");
  check(BlockInfo(5, "E"_H), 7_W, BlockInfo(9, "ED"_H), "From E with weight 7");
  check(BlockInfo(6, "F"_H), 7_W, BlockInfo(5, "E"_H),   "From F with weight 7");

  check(boost::none,         5_W, BlockInfo(9, "ED"_H), "From base with weight 5");
  check(BlockInfo(4, "D"_H), 5_W, BlockInfo(9, "ED"_H), "From D with weight 5");
  check(BlockInfo(5, "E"_H), 5_W, BlockInfo(9, "ED"_H), "From E with weight 5");
  check(BlockInfo(6, "F"_H), 5_W, BlockInfo(9, "FC"_H), "From F with weight 5");
}
