/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "core/consensus/grandpa/vote_graph/fixture.hpp"

TEST_F(VoteGraphFixture, AdjustBase) {
  BlockInfo base{6, "E"_H};
  graph = std::make_shared<VoteGraphImpl>(base, chain);
  auto &entries = graph->getEntries();

  SCOPED_TRACE(1);
  AssertGraphCorrect(*graph,
                     R"({
  "entries": {
    "E": {
      "number": 6,
      "ancestors": [],
      "descendents": [],
      "cumulative_vote": 0
    }
  },
  "heads": [
    "E"
  ],
  "base": "E",
  "base_number": 6
})");

  expect_getAncestry("E"_H, "FC"_H, vec("FB"_H, "FA"_H, "F"_H));
  EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{10, "FC"_H}, 5_W));
  SCOPED_TRACE(2);
  AssertGraphCorrect(*graph,
                     R"({
  "entries": {
    "E": {
      "number": 6,
      "ancestors": [],
      "descendents": [
        "FC"
      ],
      "cumulative_vote": 5
    },
    "FC": {
      "number": 10,
      "ancestors": [
        "FB",
        "FA",
        "F",
        "E"
      ],
      "descendents": [],
      "cumulative_vote": 5
    }
  },
  "heads": [
    "FC"
  ],
  "base": "E",
  "base_number": 6
})");

  expect_getAncestry("E"_H, "ED"_H, vec("EC"_H, "EB"_H, "EA"_H));
  EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{10, "ED"_H}, 7_W));
  SCOPED_TRACE(3);
  AssertGraphCorrect(*graph,
                     R"({
  "entries": {
    "E": {
      "number": 6,
      "ancestors": [],
      "descendents": [
        "FC",
        "ED"
      ],
      "cumulative_vote": 12
    },
    "FC": {
      "number": 10,
      "ancestors": [
        "FB",
        "FA",
        "F",
        "E"
      ],
      "descendents": [],
      "cumulative_vote": 5
    },
    "ED": {
      "number": 10,
      "ancestors": [
        "EC",
        "EB",
        "EA",
        "E"
      ],
      "descendents": [],
      "cumulative_vote": 7
    }
  },
  "heads": [
    "FC",
    "ED"
  ],
  "base": "E",
  "base_number": 6
})");

  ASSERT_EQ(graph->getBase(), BlockInfo(6, "E"_H));

  graph->adjustBase(vec("D"_H, "C"_H, "B"_H, "A"_H));
  ASSERT_EQ(graph->getBase(), BlockInfo(2, "A"_H));
  SCOPED_TRACE(4);
  AssertGraphCorrect(*graph,
                     R"({
  "entries": {
    "ED": {
      "number": 10,
      "ancestors": [
        "EC",
        "EB",
        "EA",
        "E"
      ],
      "descendents": [],
      "cumulative_vote": 7
    },
    "A": {
      "number": 2,
      "ancestors": [],
      "descendents": [
        "E"
      ],
      "cumulative_vote": 12
    },
    "E": {
      "number": 6,
      "ancestors": [
        "D",
        "C",
        "B",
        "A"
      ],
      "descendents": [
        "FC",
        "ED"
      ],
      "cumulative_vote": 12
    },
    "FC": {
      "number": 10,
      "ancestors": [
        "FB",
        "FA",
        "F",
        "E"
      ],
      "descendents": [],
      "cumulative_vote": 5
    }
  },
  "heads": [
    "FC",
    "ED"
  ],
  "base": "A",
  "base_number": 2
})");

  graph->adjustBase(std::vector<BlockHash>{GENESIS_HASH});
  ASSERT_EQ(graph->getBase(), BlockInfo(1, GENESIS_HASH));
  SCOPED_TRACE(5);
  AssertGraphCorrect(*graph,
                     R"({
  "entries": {
    "ED": {
      "number": 10,
      "ancestors": [
        "EC",
        "EB",
        "EA",
        "E"
      ],
      "descendents": [],
      "cumulative_vote": 7
    },
    "A": {
      "number": 2,
      "ancestors": [
        "genesis"
      ],
      "descendents": [
        "E"
      ],
      "cumulative_vote": 12
    },
    "E": {
      "number": 6,
      "ancestors": [
        "D",
        "C",
        "B",
        "A"
      ],
      "descendents": [
        "FC",
        "ED"
      ],
      "cumulative_vote": 12
    },
    "FC": {
      "number": 10,
      "ancestors": [
        "FB",
        "FA",
        "F",
        "E"
      ],
      "descendents": [],
      "cumulative_vote": 5
    },
    "genesis": {
      "number": 1,
      "ancestors": [],
      "descendents": [
        "A"
      ],
      "cumulative_vote": 12
    }
  },
  "heads": [
    "FC",
    "ED"
  ],
  "base": "genesis",
  "base_number": 1
})");

  ASSERT_EQ(entries[GENESIS_HASH].cumulative_vote, 12_W);

  expect_getAncestry(GENESIS_HASH, "5"_H, vec("4"_H, "3"_H, "A"_H));
  EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo(5, "5"_H), 3_W));

  SCOPED_TRACE(6);
  AssertGraphCorrect(*graph,
                     R"({
  "entries": {
    "5": {
      "number": 5,
      "ancestors": [
        "4",
        "3",
        "A"
      ],
      "descendents": [],
      "cumulative_vote": 3
    },
    "ED": {
      "number": 10,
      "ancestors": [
        "EC",
        "EB",
        "EA",
        "E"
      ],
      "descendents": [],
      "cumulative_vote": 7
    },
    "A": {
      "number": 2,
      "ancestors": [
        "genesis"
      ],
      "descendents": [
        "E",
        "5"
      ],
      "cumulative_vote": 15
    },
    "E": {
      "number": 6,
      "ancestors": [
        "D",
        "C",
        "B",
        "A"
      ],
      "descendents": [
        "FC",
        "ED"
      ],
      "cumulative_vote": 12
    },
    "FC": {
      "number": 10,
      "ancestors": [
        "FB",
        "FA",
        "F",
        "E"
      ],
      "descendents": [],
      "cumulative_vote": 5
    },
    "genesis": {
      "number": 1,
      "ancestors": [],
      "descendents": [
        "A"
      ],
      "cumulative_vote": 15
    }
  },
  "heads": [
    "5",
    "FC",
    "ED"
  ],
  "base": "genesis",
  "base_number": 1
})");
}
