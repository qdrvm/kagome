/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "core/consensus/grandpa/vote_graph/fixture.hpp"

struct WalkBackFromBlockNodeBelow
    : public VoteGraphFixture,
      public ::testing::WithParamInterface<BlockInfo> {
  const BlockInfo EXPECTED = BlockInfo(5, "D"_H);

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

    expect_getAncestry(GENESIS_HASH, "B"_H, vec("A"_H));
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{3, "B"_H}, "10"_W));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "genesis": {
      "number": 1,
      "ancestors": [],
      "descendents": [
        "B"
      ],
      "cumulative_vote": 10
    },
    "B": {
      "number": 3,
      "ancestors": [
        "A",
        "genesis"
      ],
      "descendents": [],
      "cumulative_vote": 10
    }
  },
  "heads": [
    "B"
  ],
  "base": "genesis",
  "base_number": 1
})");

    expect_getAncestry(
        GENESIS_HASH, "F1"_H, vec("E1"_H, "D"_H, "C"_H, "B"_H, "A"_H));
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{7, "F1"_H}, "5"_W));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "genesis": {
      "number": 1,
      "ancestors": [],
      "descendents": [
        "B"
      ],
      "cumulative_vote": 15
    },
    "F1": {
      "number": 7,
      "ancestors": [
        "E1",
        "D",
        "C",
        "B"
      ],
      "descendents": [],
      "cumulative_vote": 5
    },
    "B": {
      "number": 3,
      "ancestors": [
        "A",
        "genesis"
      ],
      "descendents": [
        "F1"
      ],
      "cumulative_vote": 15
    }
  },
  "heads": [
    "F1"
  ],
  "base": "genesis",
  "base_number": 1
})");

    expect_getAncestry(
        GENESIS_HASH, "G2"_H, vec("F2"_H, "E2"_H, "D"_H, "C"_H, "B"_H, "A"_H));
    EXPECT_OUTCOME_TRUE_1(graph->insert(BlockInfo{8, "G2"_H}, "5"_W));

    AssertGraphCorrect(*graph,
                       R"({
  "entries": {
    "genesis": {
      "number": 1,
      "ancestors": [],
      "descendents": [
        "B"
      ],
      "cumulative_vote": 20
    },
    "F1": {
      "number": 7,
      "ancestors": [
        "E1",
        "D",
        "C",
        "B"
      ],
      "descendents": [],
      "cumulative_vote": 5
    },
    "B": {
      "number": 3,
      "ancestors": [
        "A",
        "genesis"
      ],
      "descendents": [
        "F1",
        "G2"
      ],
      "cumulative_vote": 20
    },
    "G2": {
      "number": 8,
      "ancestors": [
        "F2",
        "E2",
        "D",
        "C",
        "B"
      ],
      "descendents": [],
      "cumulative_vote": 5
    }
  },
  "heads": [
    "F1",
    "G2"
  ],
  "base": "genesis",
  "base_number": 1
})");
  }
};

TEST_P(WalkBackFromBlockNodeBelow, FindAncestor) {
  BlockInfo block = GetParam();
  auto ancestorOpt =
      graph->findAncestor(block, [](auto &&x) { return x > "5"_W; });

  ASSERT_TRUE(ancestorOpt) << "number: " << block.block_number << " "
                           << "hash: " << block.block_hash.toHex();
  ASSERT_EQ(*ancestorOpt, EXPECTED);
}

const std::vector<BlockInfo> test_cases = {{
    // clang-format off
   BlockInfo(6, "E1"_H),
   BlockInfo(6, "E2"_H),
   BlockInfo(7, "F1"_H),
   BlockInfo(7, "F2"_H),
   BlockInfo(8, "G2"_H)
    // clang-format on
}};

INSTANTIATE_TEST_CASE_P(VoteGraph,
                        WalkBackFromBlockNodeBelow,
                        testing::ValuesIn(test_cases));

TEST_F(WalkBackFromBlockNodeBelow, GhostFindMergePoint_NoConstrain) {
  BlockHash node_key = "B"_H;
  auto &entries = graph->getEntries();
  auto &active_node = entries[node_key];
  auto subchain = graph->ghostFindMergePoint(
      node_key, active_node, boost::none, [](auto &&x) -> bool {
        return x > "5"_W;
      });

  EXPECT_EQ(subchain.best_number, 5);
  EXPECT_EQ(subchain.hashes, (vec("B"_H, "C"_H, "D"_H)));
}
