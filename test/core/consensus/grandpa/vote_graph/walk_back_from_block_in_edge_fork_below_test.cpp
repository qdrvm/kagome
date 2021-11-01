/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "core/consensus/grandpa/vote_graph/fixture.hpp"

struct WalkBackFromBlockInEdgeForkBelow
    : public VoteGraphFixture,
      public testing::WithParamInterface<BlockInfo> {
  const BlockInfo EXPECTED = BlockInfo(3, "C"_H);

  void SetUp() override {
    BlockInfo base{0, GENESIS_HASH};
    graph = std::make_shared<VoteGraphImpl>(base, voter_set, chain);

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

    expect_getAncestry(GENESIS_HASH, "B"_H, vec("B"_H, "A"_H, GENESIS_HASH));
    EXPECT_OUTCOME_TRUE_1(graph->insert({2, "B"_H}, "w10_a"_ID));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "B": {
      "number": 2,
      "ancestors": [
        "A",
        "genesis"
      ],
      "descendants": [],
      "cumulative_vote": 10
    },
    "genesis": {
      "number": 0,
      "ancestors": [],
      "descendants": [
        "B"
      ],
      "cumulative_vote": 10
    }
  },
  "heads": [
    "B"
  ],
  "base": "genesis",
  "base_number": 0
})");

    expect_getAncestry(
        GENESIS_HASH,
        "F1"_H,
        vec("F1"_H, "E1"_H, "D1"_H, "C"_H, "B"_H, "A"_H, GENESIS_HASH));
    EXPECT_OUTCOME_TRUE_1(graph->insert({6, "F1"_H}, "w5_a"_ID));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "B": {
      "number": 2,
      "ancestors": [
        "A",
        "genesis"
      ],
      "descendants": [
        "F1"
      ],
      "cumulative_vote": 15
    },
    "F1": {
      "number": 6,
      "ancestors": [
        "E1",
        "D1",
        "C",
        "B"
      ],
      "descendants": [],
      "cumulative_vote": 5
    },
    "genesis": {
      "number": 0,
      "ancestors": [],
      "descendants": [
        "B"
      ],
      "cumulative_vote": 15
    }
  },
  "heads": [
    "F1"
  ],
  "base": "genesis",
  "base_number": 0
})");

    expect_getAncestry(
        GENESIS_HASH,
        "G2"_H,
        vec("G2"_H, "F2"_H, "E2"_H, "D2"_H, "C"_H, "B"_H, "A"_H, GENESIS_HASH));
    EXPECT_OUTCOME_TRUE_1(graph->insert({7, "G2"_H}, "w5_b"_ID));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "G2": {
      "number": 7,
      "ancestors": [
        "F2",
        "E2",
        "D2",
        "C",
        "B"
      ],
      "descendants": [],
      "cumulative_vote": 5
    },
    "F1": {
      "number": 6,
      "ancestors": [
        "E1",
        "D1",
        "C",
        "B"
      ],
      "descendants": [],
      "cumulative_vote": 5
    },
    "B": {
      "number": 2,
      "ancestors": [
        "A",
        "genesis"
      ],
      "descendants": [
        "F1",
        "G2"
      ],
      "cumulative_vote": 20
    },
    "genesis": {
      "number": 0,
      "ancestors": [],
      "descendants": [
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
  "base_number": 0
})");
  }
};

TEST_P(WalkBackFromBlockInEdgeForkBelow, FindAncestor) {
  BlockInfo block = GetParam();
  auto ancestorOpt =
      graph->findAncestor(block, [](auto &&x) { return x.sum > 5; });

  ASSERT_TRUE(ancestorOpt) << "number: " << block.number << " "
                           << "hash: " << block.hash.toHex();
  ASSERT_EQ(*ancestorOpt, EXPECTED);
}

const std::vector<BlockInfo> test_cases = {{
    // clang-format off
   BlockInfo(4, "D1"_H),
   BlockInfo(4, "D2"_H),
   BlockInfo(5, "E1"_H),
   BlockInfo(5, "E2"_H),
   BlockInfo(6, "F1"_H),
   BlockInfo(6, "F2"_H),
   BlockInfo(7, "G2"_H)
    // clang-format on
}};

INSTANTIATE_TEST_SUITE_P(VoteGraph,
                         WalkBackFromBlockInEdgeForkBelow,
                         testing::ValuesIn(test_cases));
