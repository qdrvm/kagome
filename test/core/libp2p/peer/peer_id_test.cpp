/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "libp2p/peer/peer_id.hpp"
#include "common/buffer.hpp"

using namespace libp2p::multi;
using namespace libp2p::peer;
using namespace kagome::common;

class PeerIdTest : public ::testing::Test {
 public:
  Multihash valid_peer_id =
      Multihash::create(HashType::sha256, Buffer{0xAA, 0xBB}).value();
  Multihash invalid_peer_id =
      Multihash::create(HashType::blake2s128, Buffer{0xAA, 0xBB}).value();
};

/**
 * @given valid PeerId multihash
 * @when initializing PeerId from that multihash
 * @then initialization succeds
 */
TEST_F(PeerIdTest, CreateSuccess) {
  auto peer_id = PeerId::createPeerId(valid_peer_id);

  ASSERT_TRUE(peer_id);
  ASSERT_EQ(peer_id.value().getPeerId(), valid_peer_id);
}

/**
 * @given invalid PeerId multihash
 * @when initializing PeerId from that multihash
 * @then initialization fails
 */
TEST_F(PeerIdTest, CreateInvalidId) {
  auto peer_id = PeerId::createPeerId(invalid_peer_id);

  ASSERT_FALSE(peer_id);
}
