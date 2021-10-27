/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "core/consensus/grandpa/vote_graph/fixture.hpp"

TEST_F(VoteGraphFixture, GhostMergeAtNodes) {
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

    expect_getAncestry(GENESIS_HASH, "B"_H, vec("B"_H, "A"_H, GENESIS_HASH));
    EXPECT_OUTCOME_TRUE_1(graph->insert({2, "B"_H}, "w0_a"_ID));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "genesis": {
      "number": 0,
      "ancestors": [],
      "descendants": [
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
      "descendants": [],
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
    EXPECT_OUTCOME_TRUE_1(graph->insert({3, "C"_H}, "w5_a"_ID));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "genesis": {
      "number": 0,
      "ancestors": [],
      "descendants": [
        "B"
      ],
      "cumulative_vote": 5
    },
    "B": {
      "number": 2,
      "ancestors": [
        "A",
        "genesis"
      ],
      "descendants": [
        "C"
      ],
      "cumulative_vote": 5
    },
    "C": {
      "number": 3,
      "ancestors": [
        "B"
      ],
      "descendants": [],
      "cumulative_vote": 5
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
    EXPECT_OUTCOME_TRUE_1(graph->insert({5, "E1"_H}, "w5_b"_ID));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "C": {
      "number": 3,
      "ancestors": [
        "B"
      ],
      "descendants": [
        "E1"
      ],
      "cumulative_vote": 10
    },
    "B": {
      "number": 2,
      "ancestors": [
        "A",
        "genesis"
      ],
      "descendants": [
        "C"
      ],
      "cumulative_vote": 10
    },
    "genesis": {
      "number": 0,
      "ancestors": [],
      "descendants": [
        "B"
      ],
      "cumulative_vote": 10
    },
    "E1": {
      "number": 5,
      "ancestors": [
        "D1",
        "C"
      ],
      "descendants": [],
      "cumulative_vote": 5
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
    EXPECT_OUTCOME_TRUE_1(graph->insert({6, "F2"_H}, "w5_c"_ID));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "C": {
      "number": 3,
      "ancestors": [
        "B"
      ],
      "descendants": [
        "E1",
        "F2"
      ],
      "cumulative_vote": 15
    },
    "B": {
      "number": 2,
      "ancestors": [
        "A",
        "genesis"
      ],
      "descendants": [
        "C"
      ],
      "cumulative_vote": 15
    },
    "genesis": {
      "number": 0,
      "ancestors": [],
      "descendants": [
        "B"
      ],
      "cumulative_vote": 15
    },
    "E1": {
      "number": 5,
      "ancestors": [
        "D1",
        "C"
      ],
      "descendants": [],
      "cumulative_vote": 5
    },
    "F2": {
      "number": 6,
      "ancestors": [
        "E2",
        "D2",
        "C"
      ],
      "descendants": [],
      "cumulative_vote": 5
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
    auto ghostOpt =
        graph->findGhost(block, [](auto &&x) { return x.sum >= 7; });
    ASSERT_TRUE(ghostOpt);
    BlockInfo EXPECTED(3, "C"_H);
    ASSERT_EQ(*ghostOpt, EXPECTED);
  };

  check(boost::none);
  check(BlockInfo(3, "C"_H));
  check(BlockInfo(2, "B"_H));
}
