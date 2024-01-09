/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "consensus/sassafras/types/slot_claim.hpp"

TEST(X, X) {
  std::array<uint8_t, 112> x{
      0,   0,   0,   0,  // auth id (4)

      39,  132, 221, 33,  0,   0,   0,   0,  // slot (16)

      4,  // outputs size (1)

      104, 57,  127, 45,  80,  13,  242, 182, 82,  145, 111, 88,  13,  179,
      109, 96,  100, 23,  100, 235, 62,  30,  255, 172, 20,  41,  38,  205,
      208, 145, 80,  0,   128,  // output#0 (33)

      238, 44,  125, 166, 72,  62,  54,  80,  14,  149, 76,  160, 250, 68,
      161, 104, 154, 30,  187, 197, 206, 203, 181, 245, 169, 64,  177, 48,
      135, 238, 19,  5,   95,  37,  210, 248, 209, 40,  75,  133, 97,  185,
      224, 117, 16,  240, 36,  33,  174, 80,  65,  232, 105, 175, 41,  24,
      220, 186, 182, 92,  232, 44,  143, 53,  0,  // signature

      0  // ticket claim
  };

  auto z = scale::decode<kagome::consensus::sassafras::SlotClaim>(x);
}
