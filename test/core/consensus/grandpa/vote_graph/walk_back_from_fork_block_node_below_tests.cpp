/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "core/consensus/grandpa/vote_graph/fixture.hpp"

struct WalkBackFromBlockNodeBelow
    : public VoteGraphFixture,
      public ::testing::WithParamInterface<std::tuple<BlockInfo, BlockInfo>> {
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
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{3, "B"_H}, 10_W));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "genesis": {
      "number": 1,
      "ancestors": [],
      "descendents": [
        "B"
      ],
      "cumulative_vote": 10
    },
    "B": {
      "number": 3,
      "ancestors": [
        "A",
        "genesis"
      ],
      "descendents": [],
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
        GENESIS_HASH, "F1"_H, vec("E1"_H, "D"_H, "C"_H, "B"_H, "A"_H));
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{7, "F1"_H}, 5_W));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "genesis": {
      "number": 1,
      "ancestors": [],
      "descendents": [
        "B"
      ],
      "cumulative_vote": 15
    },
    "F1": {
      "number": 7,
      "ancestors": [
        "E1",
        "D",
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
        "F1"
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
        GENESIS_HASH, "G2"_H, vec("F2"_H, "E2"_H, "D"_H, "C"_H, "B"_H, "A"_H));
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{8, "G2"_H}, 5_W));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "genesis": {
      "number": 1,
      "ancestors": [],
      "descendents": [
        "B"
      ],
      "cumulative_vote": 20
    },
    "F1": {
      "number": 7,
      "ancestors": [
        "E1",
        "D",
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
    "G2": {
      "number": 8,
      "ancestors": [
        "F2",
        "E2",
        "D",
        "C",
        "B"
      ],
      "descendents": [],
      "cumulative_vote": 5
    }
  },
  "heads": [
    "F1",
    "G2"
  ],
  "base": "genesis",
  "base_number": 1
})");

    expect_getAncestry(GENESIS_HASH,
                       "H2"_H,
                       vec("G2"_H, "F2"_H, "E2"_H, "D"_H, "C"_H, "B"_H, "A"_H));
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{9, "H2"_H}, 1_W));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "genesis": {
      "number": 1,
      "ancestors": [],
      "descendents": [
        "B"
      ],
      "cumulative_vote": 21
    },
    "F1": {
      "number": 7,
      "ancestors": [
        "E1",
        "D",
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
      "cumulative_vote": 21
    },
    "G2": {
      "number": 8,
      "ancestors": [
        "F2",
        "E2",
        "D",
        "C",
        "B"
      ],
      "descendents": [
				"H2"
			],
      "cumulative_vote": 6
    },
    "H2": {
      "number": 9,
      "ancestors": [
        "G2"
      ],
      "descendents": [],
      "cumulative_vote": 1
    }
  },
  "heads": [
    "F1",
    "H2"
  ],
  "base": "genesis",
  "base_number": 1
})");
  }
};

TEST_P(WalkBackFromBlockNodeBelow, FindAncestor) {
  auto &[block, ancestor] = GetParam();
  auto ancestorOpt = graph->findAncestor(
      block,
      [](auto &&x) { return x.prevotes_sum > (5_W).prevotes_sum; },
      comparator);

  ASSERT_TRUE(ancestorOpt) << "number: " << block.block_number << " "
                           << "hash: " << block.block_hash.toHex();
  ASSERT_EQ(*ancestorOpt, ancestor);
}

const std::vector<std::tuple<BlockInfo, BlockInfo>> test_cases{{
    // clang-format off
		{{5,  "D"_H}, {5, "D"_H}},
		{{6, "E1"_H}, {5, "D"_H}},
		{{6, "E2"_H}, {5, "D"_H}},
		{{7, "F1"_H}, {5, "D"_H}},
		{{7, "F2"_H}, {5, "D"_H}},
		{{8, "G2"_H}, {8, "G2"_H}},
		{{9, "H2"_H}, {8, "G2"_H}},
    // clang-format on
}};

INSTANTIATE_TEST_CASE_P(VoteGraph,
                        WalkBackFromBlockNodeBelow,
                        testing::ValuesIn(test_cases));

TEST_F(WalkBackFromBlockNodeBelow, GhostFindMergePoint_NoConstrain) {
  BlockHash node_key = "B"_H;
  auto &entries = graph->getEntries();
  auto &active_node = entries[node_key];
  auto subchain = graph->ghostFindMergePoint(
      node_key,
      active_node,
      boost::none,
      [](auto &&x) -> bool { return x.prevotes_sum > (5_W).prevotes_sum; },
      comparator);

  EXPECT_EQ(subchain.best_number, 5);
  EXPECT_EQ(subchain.hashes, (vec("B"_H, "C"_H, "D"_H)));
}
