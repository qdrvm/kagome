/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "core/consensus/grandpa/vote_graph/fixture.hpp"

TEST_F(VoteGraphFixture, GhostIntroduceBranch) {
  BlockInfo base{0, GENESIS_HASH};
  graph = std::make_shared<VoteGraphImpl>(base, voter_set, chain);

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
    EXPECT_OUTCOME_TRUE_1(graph->insert(vt, {9, "FC"_H}, "w5_a"_ID));

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
    EXPECT_OUTCOME_TRUE_1(graph->insert(vt, {9, "ED"_H}, "w7_a"_ID));

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
        vt, std::nullopt, [](auto &&x) { return x.sum(vt) >= 5; });
    ASSERT_TRUE(ghostOpt);
    ASSERT_EQ(*ghostOpt, BlockInfo(9, "ED"_H))
        << "The best block of blocks with enough weight should be selected";
  }

  {
    auto ghostOpt = graph->findGhost(
        vt, std::nullopt, [](auto &&x) { return x.sum(vt) >= 10; });
    ASSERT_TRUE(ghostOpt);
    ASSERT_EQ(*ghostOpt, BlockInfo(5, "E"_H))
        << "A highest-weighted of blocks with enough weight should be selected";
  }

  {  // introduce branch in the middle
    // do not expect that insert is calling getAncestry
    EXPECT_OUTCOME_TRUE_1(graph->insert(vt, {5, "E"_H}, "w3_a"_ID));

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

  auto check = [&](const std::optional<BlockInfo> &block,
                   size_t weight,
                   const std::optional<BlockInfo> &expected,
                   std::string_view comment) {
    auto ghostOpt = graph->findGhost(
        vt, block, [&weight](auto &&x) { return x.sum(vt) >= weight; });
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
  check(std::nullopt,        10, BlockInfo(5, "E"_H),  "From base with weight 10");
  check(BlockInfo(4, "D"_H), 10, BlockInfo(5, "E"_H),  "From D with weight 10");
  check(BlockInfo(5, "E"_H), 10, BlockInfo(5, "E"_H),  "From E with weight 10");
  check(BlockInfo(6, "F"_H), 10, BlockInfo(5, "E"_H),  "From F with weight 10");

  check(std::nullopt,        7, BlockInfo(9, "ED"_H), "From base with weight 7");
  check(BlockInfo(4, "D"_H), 7, BlockInfo(9, "ED"_H), "From D with weight 7");
  check(BlockInfo(5, "E"_H), 7, BlockInfo(9, "ED"_H), "From E with weight 7");
  check(BlockInfo(6, "F"_H), 7, BlockInfo(5, "E"_H),   "From F with weight 7");

  check(std::nullopt,        5, BlockInfo(9, "ED"_H), "From base with weight 5");
  check(BlockInfo(4, "D"_H), 5, BlockInfo(9, "ED"_H), "From D with weight 5");
  check(BlockInfo(5, "E"_H), 5, BlockInfo(9, "ED"_H), "From E with weight 5");
  check(BlockInfo(6, "F"_H), 5, BlockInfo(9, "FC"_H), "From F with weight 5");
}
