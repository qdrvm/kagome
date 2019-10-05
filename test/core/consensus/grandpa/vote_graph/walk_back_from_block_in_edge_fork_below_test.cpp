/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "core/consensus/grandpa/vote_graph/fixture.hpp"

struct WalkBackFromBlockInEdgeForkBelow
    : public VoteGraphFixture,
      public ::testing::WithParamInterface<BlockInfo> {
  const BlockInfo EXPECTED = BlockInfo(4, "C"_H);

  void SetUp() override {
    BlockInfo base{1, GENESIS_HASH};
    graph = std::make_shared<VoteGraphImpl>(base, chain);

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

    expect_getAncestry(GENESIS_HASH, "B"_H, vec("A"_H));
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{3, "B"_H}, "10"_W));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "B": {
      "number": 3,
      "ancestors": [
        "A",
        "genesis"
      ],
      "descendents": [],
      "cumulative_vote": 10
    },
    "genesis": {
      "number": 1,
      "ancestors": [],
      "descendents": [
        "B"
      ],
      "cumulative_vote": 10
    }
  },
  "heads": [
    "B"
  ],
  "base": "genesis",
  "base_number": 1
})");

    expect_getAncestry(
        GENESIS_HASH, "F1"_H, vec("E1"_H, "D1"_H, "C"_H, "B"_H, "A"_H));
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{7, "F1"_H}, "5"_W));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "B": {
      "number": 3,
      "ancestors": [
        "A",
        "genesis"
      ],
      "descendents": [
        "F1"
      ],
      "cumulative_vote": 15
    },
    "F1": {
      "number": 7,
      "ancestors": [
        "E1",
        "D1",
        "C",
        "B"
      ],
      "descendents": [],
      "cumulative_vote": 5
    },
    "genesis": {
      "number": 1,
      "ancestors": [],
      "descendents": [
        "B"
      ],
      "cumulative_vote": 15
    }
  },
  "heads": [
    "F1"
  ],
  "base": "genesis",
  "base_number": 1
})");

    expect_getAncestry(
        GENESIS_HASH, "G2"_H, vec("F2"_H, "E2"_H, "D2"_H, "C"_H, "B"_H, "A"_H));
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{8, "G2"_H}, "5"_W));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "G2": {
      "number": 8,
      "ancestors": [
        "F2",
        "E2",
        "D2",
        "C",
        "B"
      ],
      "descendents": [],
      "cumulative_vote": 5
    },
    "F1": {
      "number": 7,
      "ancestors": [
        "E1",
        "D1",
        "C",
        "B"
      ],
      "descendents": [],
      "cumulative_vote": 5
    },
    "B": {
      "number": 3,
      "ancestors": [
        "A",
        "genesis"
      ],
      "descendents": [
        "F1",
        "G2"
      ],
      "cumulative_vote": 20
    },
    "genesis": {
      "number": 1,
      "ancestors": [],
      "descendents": [
        "B"
      ],
      "cumulative_vote": 20
    }
  },
  "heads": [
    "F1",
    "G2"
  ],
  "base": "genesis",
  "base_number": 1
})");
  }
};

TEST_P(WalkBackFromBlockInEdgeForkBelow, FindAncestor) {
  BlockInfo block = GetParam();
  auto ancestorOpt =
      graph->findAncestor(block, [](auto &&x) { return x > "5"_W; });

  ASSERT_TRUE(ancestorOpt) << "number: " << block.number << " "
                           << "hash: " << block.hash.toHex();
  ASSERT_EQ(*ancestorOpt, EXPECTED);
}

const std::vector<BlockInfo> test_cases = {{
    // clang-format off
   BlockInfo(5, "D1"_H),
   BlockInfo(5, "D2"_H),
   BlockInfo(6, "E1"_H),
   BlockInfo(6, "E2"_H),
   BlockInfo(7, "F1"_H),
   BlockInfo(7, "F2"_H),
   BlockInfo(8, "G2"_H)
    // clang-format on
}};

INSTANTIATE_TEST_CASE_P(VoteGraph,
                        WalkBackFromBlockInEdgeForkBelow,
                        testing::ValuesIn(test_cases));
