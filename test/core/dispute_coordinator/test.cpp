/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dispute_coordinator/types.hpp"

#include <gtest/gtest.h>

using namespace kagome::dispute;

TEST(X, X) {
  DisputeStatement statement{ValidDisputeStatement{BackingSeconded{}}};

  CandidateHash hash{};
  hash[31] = '\xaa';

  auto data = getSignablePayload(statement, hash, 255);

  (void)data;
}
