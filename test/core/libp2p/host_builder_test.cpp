/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/host_builder.hpp"

#include <gtest/gtest.h>

using namespace libp2p;
using namespace libp2p::multi;

class HostBuilderTest : public ::testing::Test {};

/**
 * @given host builder
 * @when building a host with no parameters
 * @then build does not fail
 */
TEST_F(HostBuilderTest, AllDefaults) {
  EXPECT_TRUE(HostBuilder{}.build());
}
