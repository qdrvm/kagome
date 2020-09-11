/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "core/consensus/grandpa/vote_graph/fixture.hpp"

TEST_F(VoteGraphFixture, GraphForkAtNode) {
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
  EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{4, "C"_H}, 100_W));

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
      "descendents": [],
      "cumulative_vote": 100
    },
    "genesis": {
      "number": 1,
      "ancestors": [],
      "descendents": [
        "C"
      ],
      "cumulative_vote": 100
    }
  },
  "heads": [
    "C"
  ],
  "base": "genesis",
  "base_number": 1
})");

  expect_getAncestry(GENESIS_HASH, "E1"_H, vec("D1"_H, "C"_H, "B"_H, "A"_H));
  EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{6, "E1"_H}, 100_W));

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
        "E1"
      ],
      "cumulative_vote": 200
    },
    "genesis": {
      "number": 1,
      "ancestors": [],
      "descendents": [
        "C"
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

  expect_getAncestry(
      GENESIS_HASH, "F2"_H, vec("E2"_H, "D2"_H, "C"_H, "B"_H, "A"_H));
  EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{7, "F2"_H}, 100_W));

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
        "E1",
        "F2"
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
    "genesis": {
      "number": 1,
      "ancestors": [],
      "descendents": [
        "C"
      ],
      "cumulative_vote": 300
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
    "E1",
    "F2"
  ],
  "base": "genesis",
  "base_number": 1
})");
}

TEST_F(VoteGraphFixture, GraphForkNotAtNode) {
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

  expect_getAncestry(GENESIS_HASH, "A"_H, vec() /* empty */);
  EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{2, "A"_H}, 100_W));

  AssertGraphCorrect(*graph,
                     R"({
  "entries": {
    "A": {
      "number": 2,
      "ancestors": [
        "genesis"
      ],
      "descendents": [],
      "cumulative_vote": 100
    },
    "genesis": {
      "number": 1,
      "ancestors": [],
      "descendents": [
        "A"
      ],
      "cumulative_vote": 100
    }
  },
  "heads": [
    "A"
  ],
  "base": "genesis",
  "base_number": 1
})");

  expect_getAncestry(GENESIS_HASH, "E1"_H, vec("D1"_H, "C"_H, "B"_H, "A"_H));
  EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{6, "E1"_H}, 100_W));

  AssertGraphCorrect(*graph,
                     R"({
  "entries": {
    "E1": {
      "number": 6,
      "ancestors": [
        "D1",
        "C",
        "B",
        "A"
      ],
      "descendents": [],
      "cumulative_vote": 100
    },
    "A": {
      "number": 2,
      "ancestors": [
        "genesis"
      ],
      "descendents": [
        "E1"
      ],
      "cumulative_vote": 200
    },
    "genesis": {
      "number": 1,
      "ancestors": [],
      "descendents": [
        "A"
      ],
      "cumulative_vote": 200
    }
  },
  "heads": [
    "E1"
  ],
  "base": "genesis",
  "base_number": 1
})");

  expect_getAncestry(
      GENESIS_HASH, "F2"_H, vec("E2"_H, "D2"_H, "C"_H, "B"_H, "A"_H));
  EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{7, "F2"_H}, 100_W));

  AssertGraphCorrect(*graph,
                     R"({
  "entries": {
    "E1": {
      "number": 6,
      "ancestors": [
        "D1",
        "C",
        "B",
        "A"
      ],
      "descendents": [],
      "cumulative_vote": 100
    },
    "A": {
      "number": 2,
      "ancestors": [
        "genesis"
      ],
      "descendents": [
        "E1",
        "F2"
      ],
      "cumulative_vote": 300
    },
    "genesis": {
      "number": 1,
      "ancestors": [],
      "descendents": [
        "A"
      ],
      "cumulative_vote": 300
    },
    "F2": {
      "number": 7,
      "ancestors": [
        "E2",
        "D2",
        "C",
        "B",
        "A"
      ],
      "descendents": [],
      "cumulative_vote": 100
    }
  },
  "heads": [
    "E1",
    "F2"
  ],
  "base": "genesis",
  "base_number": 1
})");
}
