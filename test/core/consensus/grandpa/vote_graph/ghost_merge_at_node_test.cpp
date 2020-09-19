/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "core/consensus/grandpa/vote_graph/fixture.hpp"

TEST_F(VoteGraphFixture, GhostMergeAtNodes) {
  BlockInfo base{0, GENESIS_HASH};
  graph = std::make_shared<VoteGraphImpl>(base, chain);

  {  // insert nodes
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

    expect_getAncestry(GENESIS_HASH, "B"_H, vec("B"_H, "A"_H, GENESIS_HASH));
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{2, "B"_H}, 0_W));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "genesis": {
      "number": 0,
      "ancestors": [],
      "descendents": [
        "B"
      ],
      "cumulative_vote": 0
    },
    "B": {
      "number": 2,
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
  "base_number": 0
})");

    expect_getAncestry(
        GENESIS_HASH, "C"_H, vec("C"_H, "B"_H, "A"_H, GENESIS_HASH));
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{3, "C"_H}, 100_W));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "genesis": {
      "number": 0,
      "ancestors": [],
      "descendents": [
        "B"
      ],
      "cumulative_vote": 100
    },
    "B": {
      "number": 2,
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
      "number": 3,
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
  "base_number": 0
})");

    expect_getAncestry(GENESIS_HASH,
                       "E1"_H,
                       vec("E1"_H, "D1"_H, "C"_H, "B"_H, "A"_H, GENESIS_HASH));
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{5, "E1"_H}, 100_W));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "C": {
      "number": 3,
      "ancestors": [
        "B"
      ],
      "descendents": [
        "E1"
      ],
      "cumulative_vote": 200
    },
    "B": {
      "number": 2,
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
      "number": 0,
      "ancestors": [],
      "descendents": [
        "B"
      ],
      "cumulative_vote": 200
    },
    "E1": {
      "number": 5,
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
  "base_number": 0
})");

    expect_getAncestry(
        GENESIS_HASH,
        "F2"_H,
        vec("F2"_H, "E2"_H, "D2"_H, "C"_H, "B"_H, "A"_H, GENESIS_HASH));
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{6, "F2"_H}, 100_W));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "C": {
      "number": 3,
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
      "number": 2,
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
      "number": 0,
      "ancestors": [],
      "descendents": [
        "B"
      ],
      "cumulative_vote": 300
    },
    "E1": {
      "number": 5,
      "ancestors": [
        "D1",
        "C"
      ],
      "descendents": [],
      "cumulative_vote": 100
    },
    "F2": {
      "number": 6,
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
  "base_number": 0
})");
  }

  auto check = [&](const boost::optional<BlockInfo> &block) {
    auto ghostOpt = graph->findGhost(
        block,
        [](auto &&x) { return x.prevotes_sum >= (250_W).prevotes_sum; },
        comparator);
    ASSERT_TRUE(ghostOpt);
    BlockInfo EXPECTED(3, "C"_H);
    ASSERT_EQ(*ghostOpt, EXPECTED);
  };

  check(boost::none);
  check(BlockInfo(3, "C"_H));
  check(BlockInfo(2, "B"_H));
}
