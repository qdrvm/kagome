/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/protocols/sync_protocol_impl.hpp"

#include <gtest/gtest.h>

#include "testutil/literals.hpp"

using kagome::network::BlocksRequest;
using kagome::network::detail::BlocksResponseCache;
using libp2p::peer::PeerId;

struct BlockResponseCacheTest : ::testing::Test {
  const PeerId peer1_{"peer1"_peerid};
  const BlocksRequest::Fingerprint id1_{1};
  BlocksResponseCache cache_{1, std::chrono::seconds(1)};
};

/**
 * @given two requests
 * @when third request
 * @then denied
 */
TEST_F(BlockResponseCacheTest, ThirdRequestDenied) {
  EXPECT_FALSE(cache_.isDuplicate(peer1_, id1_));
  EXPECT_FALSE(cache_.isDuplicate(peer1_, id1_));
  EXPECT_TRUE(cache_.isDuplicate(peer1_, id1_));
}
