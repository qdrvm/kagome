/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "core/consensus/grandpa/vote_graph/fixture.hpp"

TEST_F(VoteGraphFixture, AdjustBase) {
  BlockInfo base{5, "E"_H};
  graph = std::make_shared<VoteGraphImpl>(base, voter_set, chain);
  auto &entries = graph->getEntries();

  SCOPED_TRACE(1);
  AssertGraphCorrect(*graph,
                     R"({
  "entries": {
    "E": {
      "number": 5,
      "ancestors": [],
      "descendants": [],
      "cumulative_vote": 0
    }
  },
  "heads": [
    "E"
  ],
  "base": "E",
  "base_number": 5
})");

  expect_getAncestry("E"_H, "FC"_H, vec("FC"_H, "FB"_H, "FA"_H, "F"_H, "E"_H));
  EXPECT_OUTCOME_TRUE_1(graph->insert(vt, {9, "FC"_H}, "w5_a"_ID));
  SCOPED_TRACE(2);
  AssertGraphCorrect(*graph,
                     R"({
  "entries": {
    "E": {
      "number": 5,
      "ancestors": [],
      "descendants": [
        "FC"
      ],
      "cumulative_vote": 5
    },
    "FC": {
      "number": 9,
      "ancestors": [
        "FB",
        "FA",
        "F",
        "E"
      ],
      "descendants": [],
      "cumulative_vote": 5
    }
  },
  "heads": [
    "FC"
  ],
  "base": "E",
  "base_number": 5
})");

  expect_getAncestry("E"_H, "ED"_H, vec("ED"_H, "EC"_H, "EB"_H, "EA"_H, "E"_H));
  EXPECT_OUTCOME_TRUE_1(graph->insert(vt, {9, "ED"_H}, "w7_a"_ID));
  SCOPED_TRACE(3);
  AssertGraphCorrect(*graph,
                     R"({
  "entries": {
    "E": {
      "number": 5,
      "ancestors": [],
      "descendants": [
        "FC",
        "ED"
      ],
      "cumulative_vote": 12
    },
    "FC": {
      "number": 9,
      "ancestors": [
        "FB",
        "FA",
        "F",
        "E"
      ],
      "descendants": [],
      "cumulative_vote": 5
    },
    "ED": {
      "number": 9,
      "ancestors": [
        "EC",
        "EB",
        "EA",
        "E"
      ],
      "descendants": [],
      "cumulative_vote": 7
    }
  },
  "heads": [
    "FC",
    "ED"
  ],
  "base": "E",
  "base_number": 5
})");

  ASSERT_EQ(graph->getBase(), BlockInfo(5, "E"_H));

  graph->adjustBase(vec("D"_H, "C"_H, "B"_H, "A"_H));
  ASSERT_EQ(graph->getBase(), BlockInfo(1, "A"_H));
  SCOPED_TRACE(4);
  AssertGraphCorrect(*graph,
                     R"({
  "entries": {
    "ED": {
      "number": 9,
      "ancestors": [
        "EC",
        "EB",
        "EA",
        "E"
      ],
      "descendants": [],
      "cumulative_vote": 7
    },
    "A": {
      "number": 1,
      "ancestors": [],
      "descendants": [
        "E"
      ],
      "cumulative_vote": 12
    },
    "E": {
      "number": 5,
      "ancestors": [
        "D",
        "C",
        "B",
        "A"
      ],
      "descendants": [
        "FC",
        "ED"
      ],
      "cumulative_vote": 12
    },
    "FC": {
      "number": 9,
      "ancestors": [
        "FB",
        "FA",
        "F",
        "E"
      ],
      "descendants": [],
      "cumulative_vote": 5
    }
  },
  "heads": [
    "FC",
    "ED"
  ],
  "base": "A",
  "base_number": 1
})");

  graph->adjustBase(std::vector<BlockHash>{GENESIS_HASH});
  ASSERT_EQ(graph->getBase(), BlockInfo(0, GENESIS_HASH));
  SCOPED_TRACE(5);
  AssertGraphCorrect(*graph,
                     R"({
  "entries": {
    "ED": {
      "number": 9,
      "ancestors": [
        "EC",
        "EB",
        "EA",
        "E"
      ],
      "descendants": [],
      "cumulative_vote": 7
    },
    "A": {
      "number": 1,
      "ancestors": [
        "genesis"
      ],
      "descendants": [
        "E"
      ],
      "cumulative_vote": 12
    },
    "E": {
      "number": 5,
      "ancestors": [
        "D",
        "C",
        "B",
        "A"
      ],
      "descendants": [
        "FC",
        "ED"
      ],
      "cumulative_vote": 12
    },
    "FC": {
      "number": 9,
      "ancestors": [
        "FB",
        "FA",
        "F",
        "E"
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
      "cumulative_vote": 12
    }
  },
  "heads": [
    "FC",
    "ED"
  ],
  "base": "genesis",
  "base_number": 0
})");

  ASSERT_EQ(entries.at(GENESIS_HASH).cumulative_vote.sum(vt), 12);

  expect_getAncestry(
      GENESIS_HASH, "4"_H, vec("4"_H, "3"_H, "2"_H, "A"_H, GENESIS_HASH));
  EXPECT_OUTCOME_TRUE_1(graph->insert(vt, {4, "4"_H}, "w3_a"_ID));

  SCOPED_TRACE(6);
  AssertGraphCorrect(*graph,
                     R"({
  "entries": {
    "4": {
      "number": 4,
      "ancestors": [
        "3",
        "2",
        "A"
      ],
      "descendants": [],
      "cumulative_vote": 3
    },
    "ED": {
      "number": 9,
      "ancestors": [
        "EC",
        "EB",
        "EA",
        "E"
      ],
      "descendants": [],
      "cumulative_vote": 7
    },
    "A": {
      "number": 1,
      "ancestors": [
        "genesis"
      ],
      "descendants": [
        "E",
        "4"
      ],
      "cumulative_vote": 15
    },
    "E": {
      "number": 5,
      "ancestors": [
        "D",
        "C",
        "B",
        "A"
      ],
      "descendants": [
        "FC",
        "ED"
      ],
      "cumulative_vote": 12
    },
    "FC": {
      "number": 9,
      "ancestors": [
        "FB",
        "FA",
        "F",
        "E"
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
      "cumulative_vote": 15
    }
  },
  "heads": [
    "4",
    "FC",
    "ED"
  ],
  "base": "genesis",
  "base_number": 0
})");
}
