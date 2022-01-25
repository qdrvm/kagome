/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include "core/consensus/grandpa/vote_graph/fixture.hpp"

TEST_F(VoteGraphFixture, GraphForkAtNode) {
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

  expect_getAncestry(
      GENESIS_HASH, "C"_H, vec("C"_H, "B"_H, "A"_H, GENESIS_HASH));
  EXPECT_OUTCOME_TRUE_1(graph->insert(vt, {3, "C"_H}, "w5_a"_ID));

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
      "descendants": [],
      "cumulative_vote": 5
    },
    "genesis": {
      "number": 0,
      "ancestors": [],
      "descendants": [
        "C"
      ],
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
  EXPECT_OUTCOME_TRUE_1(graph->insert(vt, {5, "E1"_H}, "w5_b"_ID));

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
      "descendants": [
        "E1"
      ],
      "cumulative_vote": 10
    },
    "genesis": {
      "number": 0,
      "ancestors": [],
      "descendants": [
        "C"
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
  EXPECT_OUTCOME_TRUE_1(graph->insert(vt, {6, "F2"_H}, "w5_c"_ID));

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
      "descendants": [
        "E1",
        "F2"
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
    "genesis": {
      "number": 0,
      "ancestors": [],
      "descendants": [
        "C"
      ],
      "cumulative_vote": 15
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
    "E1",
    "F2"
  ],
  "base": "genesis",
  "base_number": 0
})");
}

TEST_F(VoteGraphFixture, GraphForkNotAtNode) {
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

  expect_getAncestry(GENESIS_HASH, "A"_H, vec("A"_H, GENESIS_HASH) /* empty */);
  EXPECT_OUTCOME_TRUE_1(graph->insert(vt, {1, "A"_H}, "w5_a"_ID));

  AssertGraphCorrect(*graph,
                     R"({
  "entries": {
    "A": {
      "number": 1,
      "ancestors": [
        "genesis"
      ],
      "descendants": [],
      "cumulative_vote": 5
    },
    "genesis": {
      "number": 0,
      "ancestors": [],
      "descendants": [
        "A"
      ],
      "cumulative_vote": 5
    }
  },
  "heads": [
    "A"
  ],
  "base": "genesis",
  "base_number": 0
})");

  expect_getAncestry(GENESIS_HASH,
                     "E1"_H,
                     vec("E1"_H, "D1"_H, "C"_H, "B"_H, "A"_H, GENESIS_HASH));
  EXPECT_OUTCOME_TRUE_1(graph->insert(vt, {5, "E1"_H}, "w5_b"_ID));

  AssertGraphCorrect(*graph,
                     R"({
  "entries": {
    "E1": {
      "number": 5,
      "ancestors": [
        "D1",
        "C",
        "B",
        "A"
      ],
      "descendants": [],
      "cumulative_vote": 5
    },
    "A": {
      "number": 1,
      "ancestors": [
        "genesis"
      ],
      "descendants": [
        "E1"
      ],
      "cumulative_vote": 10
    },
    "genesis": {
      "number": 0,
      "ancestors": [],
      "descendants": [
        "A"
      ],
      "cumulative_vote": 10
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
  EXPECT_OUTCOME_TRUE_1(graph->insert(vt, {6, "F2"_H}, "w5_c"_ID));

  AssertGraphCorrect(*graph,
                     R"({
  "entries": {
    "E1": {
      "number": 5,
      "ancestors": [
        "D1",
        "C",
        "B",
        "A"
      ],
      "descendants": [],
      "cumulative_vote": 5
    },
    "A": {
      "number": 1,
      "ancestors": [
        "genesis"
      ],
      "descendants": [
        "E1",
        "F2"
      ],
      "cumulative_vote": 15
    },
    "genesis": {
      "number": 0,
      "ancestors": [],
      "descendants": [
        "A"
      ],
      "cumulative_vote": 15
    },
    "F2": {
      "number": 6,
      "ancestors": [
        "E2",
        "D2",
        "C",
        "B",
        "A"
      ],
      "descendants": [],
      "cumulative_vote": 5
    }
  },
  "heads": [
    "E1",
    "F2"
  ],
  "base": "genesis",
  "base_number": 0
})");
}
