/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "core/consensus/grandpa/vote_graph/fixture.hpp"

struct WalkBackFromBlockNodeBelow
    : public VoteGraphFixture,
      public testing::WithParamInterface<std::tuple<BlockInfo, BlockInfo>> {
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
    EXPECT_OUTCOME_TRUE_1(graph->insert(vt, {2, "B"_H}, "w10_a"_ID));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "genesis": {
      "number": 0,
      "ancestors": [],
      "descendants": [
        "B"
      ],
      "cumulative_vote": 10
    },
    "B": {
      "number": 2,
      "ancestors": [
        "A",
        "genesis"
      ],
      "descendants": [],
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
        vec("F1"_H, "E1"_H, "D"_H, "C"_H, "B"_H, "A"_H, GENESIS_HASH));
    EXPECT_OUTCOME_TRUE_1(graph->insert(vt, {6, "F1"_H}, "w5_a"_ID));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "genesis": {
      "number": 0,
      "ancestors": [],
      "descendants": [
        "B"
      ],
      "cumulative_vote": 15
    },
    "F1": {
      "number": 6,
      "ancestors": [
        "E1",
        "D",
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
        "F1"
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
        vec("G2"_H, "F2"_H, "E2"_H, "D"_H, "C"_H, "B"_H, "A"_H, GENESIS_HASH));
    EXPECT_OUTCOME_TRUE_1(graph->insert(vt, {7, "G2"_H}, "w5_b"_ID));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "genesis": {
      "number": 0,
      "ancestors": [],
      "descendants": [
        "B"
      ],
      "cumulative_vote": 20
    },
    "F1": {
      "number": 6,
      "ancestors": [
        "E1",
        "D",
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
    "G2": {
      "number": 7,
      "ancestors": [
        "F2",
        "E2",
        "D",
        "C",
        "B"
      ],
      "descendants": [],
      "cumulative_vote": 5
    }
  },
  "heads": [
    "F1",
    "G2"
  ],
  "base": "genesis",
  "base_number": 0
})");

    expect_getAncestry(GENESIS_HASH,
                       "H2"_H,
                       vec("H2"_H,
                           "G2"_H,
                           "F2"_H,
                           "E2"_H,
                           "D"_H,
                           "C"_H,
                           "B"_H,
                           "A"_H,
                           GENESIS_HASH));
    EXPECT_OUTCOME_TRUE_1(graph->insert(vt, {8, "H2"_H}, "w1_a"_ID));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "genesis": {
      "number": 0,
      "ancestors": [],
      "descendants": [
        "B"
      ],
      "cumulative_vote": 21
    },
    "F1": {
      "number": 6,
      "ancestors": [
        "E1",
        "D",
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
      "cumulative_vote": 21
    },
    "G2": {
      "number": 7,
      "ancestors": [
        "F2",
        "E2",
        "D",
        "C",
        "B"
      ],
      "descendants": [
        "H2"
      ],
      "cumulative_vote": 6
    },
    "H2": {
      "number": 8,
      "ancestors": [
        "G2"
      ],
      "descendants": [],
      "cumulative_vote": 1
    }
  },
  "heads": [
    "F1",
    "H2"
  ],
  "base": "genesis",
  "base_number": 0
})");
  }
};

TEST_P(WalkBackFromBlockNodeBelow, FindAncestor) {
  auto &[block, expected] = GetParam();

  auto result =
      graph->findAncestor(vt, block, [](auto &&x) { return x.sum(vt) > 5; });

  ASSERT_TRUE(result.has_value())
      << '#' << block.number << ' ' << block.hash.toString().c_str()
      << " - none\n";

  const auto &actual = result.value();
  ASSERT_EQ(actual, expected)
      << '#' << block.number << ' ' << block.hash.toString().c_str() << " - "
      << "actual: #" << actual.number << ' ' << actual.hash.toString().c_str()
      << ", expected: #" << expected.number << " "
      << expected.hash.toString().c_str() << "\n";
}

const std::vector<std::tuple<BlockInfo, BlockInfo>> test_cases{{
    // clang-format off
    {{0,  "genesis"_H}, {0, "genesis"_H}},
    {{1,  "A"_H}, {1,  "A"_H}},
    {{2,  "B"_H}, {2,  "B"_H}},
    {{3,  "C"_H}, {3,  "C"_H}},
    {{4,  "D"_H}, {4,  "D"_H}},
    {{5, "E1"_H}, {4,  "D"_H}},
    {{5, "E2"_H}, {5, "E2"_H}},
    {{6, "F1"_H}, {4,  "D"_H}},
    {{6, "F2"_H}, {6, "F2"_H}},
    {{7, "G2"_H}, {7, "G2"_H}},
    {{8, "H2"_H}, {7, "G2"_H}},
    // clang-format on
}};

INSTANTIATE_TEST_SUITE_P(VoteGraph,
                         WalkBackFromBlockNodeBelow,
                         testing::ValuesIn(test_cases));

TEST_F(WalkBackFromBlockNodeBelow, GhostFindMergePoint_NoConstrain) {
  BlockHash node_key = "B"_H;
  auto &entries = graph->getEntries();
  auto &active_node = entries.at(node_key);
  auto subchain = graph->ghostFindMergePoint(
      vt, node_key, active_node, std::nullopt, [](auto &&x) -> bool {
        return x.sum(vt) > 5;
      });

  EXPECT_EQ(subchain.best_number, 4);
  EXPECT_EQ(subchain.hashes, (vec("B"_H, "C"_H, "D"_H)));
}
