/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "core/consensus/grandpa/vote_graph/fixture.hpp"

TEST_F(VoteGraphFixture, GhostMergeAtNodes) {
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

    expect_getAncestry("A"_H);
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{3, "B"_H}, "0"_W));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "genesis": {
      "number": 1,
      "ancestors": [],
      "descendents": [
        "B"
      ],
      "cumulative_vote": 0
    },
    "B": {
      "number": 3,
      "ancestors": [
        "A",
        "genesis"
      ],
      "descendents": [],
      "cumulative_vote": 0
    }
  },
  "heads": [
    "B"
  ],
  "base": "genesis",
  "base_number": 1
})");

    expect_getAncestry("B"_H, "A"_H);
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{4, "C"_H}, "100"_W));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "genesis": {
      "number": 1,
      "ancestors": [],
      "descendents": [
        "B"
      ],
      "cumulative_vote": 100
    },
    "B": {
      "number": 3,
      "ancestors": [
        "A",
        "genesis"
      ],
      "descendents": [
        "C"
      ],
      "cumulative_vote": 100
    },
    "C": {
      "number": 4,
      "ancestors": [
        "B"
      ],
      "descendents": [],
      "cumulative_vote": 100
    }
  },
  "heads": [
    "C"
  ],
  "base": "genesis",
  "base_number": 1
})");

    expect_getAncestry("D1"_H, "C"_H, "B"_H, "A"_H);
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{6, "E1"_H}, "100"_W));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "C": {
      "number": 4,
      "ancestors": [
        "B"
      ],
      "descendents": [
        "E1"
      ],
      "cumulative_vote": 200
    },
    "B": {
      "number": 3,
      "ancestors": [
        "A",
        "genesis"
      ],
      "descendents": [
        "C"
      ],
      "cumulative_vote": 200
    },
    "genesis": {
      "number": 1,
      "ancestors": [],
      "descendents": [
        "B"
      ],
      "cumulative_vote": 200
    },
    "E1": {
      "number": 6,
      "ancestors": [
        "D1",
        "C"
      ],
      "descendents": [],
      "cumulative_vote": 100
    }
  },
  "heads": [
    "E1"
  ],
  "base": "genesis",
  "base_number": 1
})");

    expect_getAncestry("E2"_H, "D2"_H, "C"_H, "B"_H, "A"_H);
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{7, "F2"_H}, "100"_W));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "C": {
      "number": 4,
      "ancestors": [
        "B"
      ],
      "descendents": [
        "E1",
        "F2"
      ],
      "cumulative_vote": 300
    },
    "B": {
      "number": 3,
      "ancestors": [
        "A",
        "genesis"
      ],
      "descendents": [
        "C"
      ],
      "cumulative_vote": 300
    },
    "genesis": {
      "number": 1,
      "ancestors": [],
      "descendents": [
        "B"
      ],
      "cumulative_vote": 300
    },
    "E1": {
      "number": 6,
      "ancestors": [
        "D1",
        "C"
      ],
      "descendents": [],
      "cumulative_vote": 100
    },
    "F2": {
      "number": 7,
      "ancestors": [
        "E2",
        "D2",
        "C"
      ],
      "descendents": [],
      "cumulative_vote": 100
    }
  },
  "heads": [
    "F2",
    "E1"
  ],
  "base": "genesis",
  "base_number": 1
})");
  }

  auto check = [&](const boost::optional<BlockInfo> &block) {
    auto ghostOpt =
        graph->findGhost(block, [](auto &&x) { return x >= "250"_W; });
    ASSERT_TRUE(ghostOpt);
    BlockInfo EXPECTED(4, "C"_H);
    ASSERT_EQ(*ghostOpt, EXPECTED);
  };

  check(boost::none);
  check(BlockInfo(4, "C"_H));
  check(BlockInfo(3, "B"_H));
}
