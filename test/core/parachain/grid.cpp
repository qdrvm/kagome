/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <boost/range/adaptor/indexed.hpp>

#include "parachain/backing/grid.hpp"

using kagome::parachain::grid::Grid;
using kagome::parachain::grid::shuffle;
using kagome::parachain::grid::ValidatorIndex;

/**
 * update_gossip_topology
 * https://github.com/paritytech/polkadot-sdk/blob/943eb46ed54c2fcd9fab693b86ef59ce18c0f792/polkadot/node/network/gossip-support/src/lib.rs#L577-L633
 */
TEST(GridTest, Shuffle) {
  size_t count = 100;
  auto randomness_hex =
      "3e3af4adec1ce3f72cae15157c2373db5aa79f03c229b26d026569bcaf94b50d";

  std::vector<std::vector<ValidatorIndex>> groups;
  groups.emplace_back(count);
  auto randomness = kagome::common::Hash256::fromHex(randomness_hex).value();
  auto indices = shuffle(groups, randomness);
  std::vector<ValidatorIndex> expected{
      48, 29, 17, 25, 16, 62, 97, 83, 89, 21, 42, 77, 93, 45, 84, 27, 91,
      65, 79, 82, 11, 99, 92, 68, 41, 28, 59, 69, 6,  80, 72, 33, 78, 20,
      96, 47, 86, 70, 94, 35, 2,  74, 26, 43, 4,  7,  44, 1,  5,  22, 53,
      19, 73, 54, 14, 0,  57, 34, 81, 37, 85, 39, 76, 90, 55, 12, 71, 88,
      60, 49, 8,  38, 50, 9,  23, 95, 13, 58, 56, 46, 3,  51, 61, 40, 87,
      52, 36, 67, 75, 98, 66, 64, 63, 24, 18, 31, 10, 32, 15, 30,
  };
  EXPECT_EQ(indices, expected);
}

/**
 * matrix_neighbors
 * https://github.com/paritytech/polkadot-sdk/blob/d5fe478e4fe2d62b0800888ae77b00ff0ba28b28/polkadot/node/network/protocol/src/grid_topology.rs#L155-L182
 */
TEST(GridTest, Cross) {
  std::vector<std::vector<size_t>> crosses{
      {1, 2, 3, 6, 9},
      {0, 2, 4, 7, 10},
      {0, 1, 5, 8},
      {0, 4, 5, 6, 9},
      {1, 3, 5, 7, 10},
      {2, 3, 4, 8},
      {0, 3, 7, 8, 9},
      {1, 4, 6, 8, 10},
      {2, 5, 6, 7},
      {0, 3, 6, 10},
      {1, 4, 7, 9},
  };
  Grid grid{crosses.size()};
  for (auto [i, expected] : boost::adaptors::index(crosses)) {
    std::vector<size_t> cross;
    grid.cross(i, [&](size_t x) { cross.emplace_back(x); });
    std::sort(cross.begin(), cross.end());
    EXPECT_EQ(cross, expected);
  }
}
