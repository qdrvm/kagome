/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol/ping.hpp"

#include <gtest/gtest.h>

class PingTest : public testing::Test {};

/**
 * @given Ping protocol handler
 * @when a stream over the Ping protocol arrives
 * @then a new session is created @and reads a Ping message from the stream @and
 * writes it back
 */
TEST_F(PingTest, PingServer) {
  
}

/**
 * @given Ping protocol handler
 * @when a stream over the Ping protocol is initiated from our side
 * @then a Ping message is sent over that stream @and we expect to get it back
 */
TEST_F(PingTest, PingClient) {}
