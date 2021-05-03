/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "core/consensus/grandpa/vote_graph/fixture.hpp"

struct WalkBackAtNode : public VoteGraphFixture,
                        public testing::WithParamInterface<BlockInfo> {
  const BlockInfo EXPECTED = BlockInfo(3, "C"_H);

  void SetUp() override {
    BlockInfo base{0, GENESIS_HASH};
    graph = std::make_shared<VoteGraphImpl>(base, chain);

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "genesis": {
      "number": 0,
      "ancestors": [],
      "descendents": [],
      "cumulative_vote": 0
    }
  },
  "heads": [
    "genesis"
  ],
  "base": "genesis",
  "base_number": 0
})");

    expect_getAncestry(
        GENESIS_HASH, "C"_H, vec("C"_H, "B"_H, "A"_H, GENESIS_HASH));
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{3, "C"_H}, 10_W));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "genesis": {
      "number": 0,
      "ancestors": [],
      "descendents": [
        "C"
      ],
      "cumulative_vote": 10
    },
    "C": {
      "number": 3,
      "ancestors": [
        "B",
        "A",
        "genesis"
      ],
      "descendents": [],
      "cumulative_vote": 10
    }
  },
  "heads": [
    "C"
  ],
  "base": "genesis",
  "base_number": 0
})");

    expect_getAncestry(
        GENESIS_HASH,
        "F1"_H,
        vec("F1"_H, "E1"_H, "D1"_H, "C"_H, "B"_H, "A"_H, GENESIS_HASH));
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{6, "F1"_H}, 5_W));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "C": {
      "number": 3,
      "ancestors": [
        "B",
        "A",
        "genesis"
      ],
      "descendents": [
        "F1"
      ],
      "cumulative_vote": 15
    },
    "genesis": {
      "number": 0,
      "ancestors": [],
      "descendents": [
        "C"
      ],
      "cumulative_vote": 15
    },
    "F1": {
      "number": 6,
      "ancestors": [
        "E1",
        "D1",
        "C"
      ],
      "descendents": [],
      "cumulative_vote": 5
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
        "F2"_H,
        vec("F2"_H, "E2"_H, "D2"_H, "C"_H, "B"_H, "A"_H, GENESIS_HASH));
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{6, "F2"_H}, 5_W));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "F1": {
      "number": 6,
      "ancestors": [
        "E1",
        "D1",
        "C"
      ],
      "descendents": [],
      "cumulative_vote": 5
    },
    "C": {
      "number": 3,
      "ancestors": [
        "B",
        "A",
        "genesis"
      ],
      "descendents": [
        "F1",
        "F2"
      ],
      "cumulative_vote": 20
    },
    "F2": {
      "number": 6,
      "ancestors": [
        "E2",
        "D2",
        "C"
      ],
      "descendents": [],
      "cumulative_vote": 5
    },
    "genesis": {
      "number": 0,
      "ancestors": [],
      "descendents": [
        "C"
      ],
      "cumulative_vote": 20
    }
  },
  "heads": [
    "F1",
    "F2"
  ],
  "base": "genesis",
  "base_number": 0
})");

    expect_getAncestry(GENESIS_HASH,
                       "I1"_H,
                       vec("I1"_H,
                           "H1"_H,
                           "G1"_H,
                           "F1"_H,
                           "E1"_H,
                           "D1"_H,
                           "C"_H,
                           "B"_H,
                           "A"_H,
                           GENESIS_HASH));
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{9, "I1"_H}, 1_W));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "F1": {
      "number": 6,
      "ancestors": [
        "E1",
        "D1",
        "C"
      ],
      "descendents": [
        "I1"
      ],
      "cumulative_vote": 6
    },
    "C": {
      "number": 3,
      "ancestors": [
        "B",
        "A",
        "genesis"
      ],
      "descendents": [
        "F1",
        "F2"
      ],
      "cumulative_vote": 21
    },
    "F2": {
      "number": 6,
      "ancestors": [
        "E2",
        "D2",
        "C"
      ],
      "descendents": [],
      "cumulative_vote": 5
    },
    "genesis": {
      "number": 0,
      "ancestors": [],
      "descendents": [
        "C"
      ],
      "cumulative_vote": 21
    },
    "I1": {
      "number": 9,
      "ancestors": [
        "H1",
        "G1",
        "F1"
      ],
      "descendents": [],
      "cumulative_vote": 1
    }
  },
  "heads": [
    "I1",
    "F2"
  ],
  "base": "genesis",
  "base_number": 0
})");
  }
};

TEST_P(WalkBackAtNode, FindAncestor) {
  BlockInfo block = GetParam();
  auto ancestorOpt = graph->findAncestor(
      block,
      [](auto x) { return x.prevotes_sum >= (20_W).prevotes_sum; },
      comparator);

  ASSERT_TRUE(ancestorOpt) << "number: " << block.number << " "
                           << "hash: " << block.hash.toHex();
  ASSERT_EQ(*ancestorOpt, EXPECTED);
}

const std::vector<BlockInfo> test_cases = {{
    // clang-format off
  BlockInfo(3, "C"_H),
  BlockInfo(4, "D1"_H),
  BlockInfo(4, "D2"_H),
  BlockInfo(5, "E1"_H),
  BlockInfo(5, "E2"_H),
  BlockInfo(6, "F1"_H),
  BlockInfo(6, "F2"_H),
  BlockInfo(9, "I1"_H)
    // clang-format onS
}};

INSTANTIATE_TEST_CASE_P(VoteGraph,
                        WalkBackAtNode,
                        testing::ValuesIn(test_cases));
