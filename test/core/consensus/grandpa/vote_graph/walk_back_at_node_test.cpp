/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "core/consensus/grandpa/vote_graph/fixture.hpp"

struct WalkBackAtNode : public VoteGraphFixture,
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

    expect_getAncestry(GENESIS_HASH, "C"_H, vec("B"_H, "A"_H));
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{4, "C"_H}, "10"_W));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "genesis": {
      "number": 1,
      "ancestors": [],
      "descendents": [
        "C"
      ],
      "cumulative_vote": 10
    },
    "C": {
      "number": 4,
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
  "base_number": 1
})");

    expect_getAncestry(
        GENESIS_HASH, "F1"_H, vec("E1"_H, "D1"_H, "C"_H, "B"_H, "A"_H));
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{7, "F1"_H}, "5"_W));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "C": {
      "number": 4,
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
      "number": 1,
      "ancestors": [],
      "descendents": [
        "C"
      ],
      "cumulative_vote": 15
    },
    "F1": {
      "number": 7,
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
  "base_number": 1
})");

    expect_getAncestry(
        GENESIS_HASH, "F2"_H, vec("E2"_H, "D2"_H, "C"_H, "B"_H, "A"_H));
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{7, "F2"_H}, "5"_W));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "F1": {
      "number": 7,
      "ancestors": [
        "E1",
        "D1",
        "C"
      ],
      "descendents": [],
      "cumulative_vote": 5
    },
    "C": {
      "number": 4,
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
      "number": 7,
      "ancestors": [
        "E2",
        "D2",
        "C"
      ],
      "descendents": [],
      "cumulative_vote": 5
    },
    "genesis": {
      "number": 1,
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
  "base_number": 1
})");

    expect_getAncestry(
        GENESIS_HASH,
        "I1"_H,
        vec("H1"_H, "G1"_H, "F1"_H, "E1"_H, "D1"_H, "C"_H, "B"_H, "A"_H));
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{10, "I1"_H}, "1"_W));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "F1": {
      "number": 7,
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
      "number": 4,
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
      "number": 7,
      "ancestors": [
        "E2",
        "D2",
        "C"
      ],
      "descendents": [],
      "cumulative_vote": 5
    },
    "genesis": {
      "number": 1,
      "ancestors": [],
      "descendents": [
        "C"
      ],
      "cumulative_vote": 21
    },
    "I1": {
      "number": 10,
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
  "base_number": 1
})");
  }
};

TEST_P(WalkBackAtNode, FindAncestor) {
  BlockInfo block = GetParam();
  auto ancestorOpt =
      graph->findAncestor(block, [](auto &&x) { return x >= "20"_W; });

  ASSERT_TRUE(ancestorOpt) << "number: " << block.block_number << " "
                           << "hash: " << block.block_hash.toHex();
  ASSERT_EQ(*ancestorOpt, EXPECTED);
}

const std::vector<BlockInfo> test_cases = {{
    // clang-format off
  BlockInfo(4, "C"_H),
  BlockInfo(5, "D1"_H),
  BlockInfo(5, "D2"_H),
  BlockInfo(6, "E1"_H),
  BlockInfo(6, "E2"_H),
  BlockInfo(7, "F1"_H),
  BlockInfo(7, "F2"_H),
  BlockInfo(10, "I1"_H)
    // clang-format onS
}};

INSTANTIATE_TEST_CASE_P(VoteGraph,
                        WalkBackAtNode,
                        testing::ValuesIn(test_cases));
