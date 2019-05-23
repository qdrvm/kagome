/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/host_builder.hpp"

#include <gtest/gtest.h>

using namespace libp2p;
using namespace libp2p::multi;

class HostBuilderTest : public ::testing::Test {
 public:
  const Multiaddress kDefaultAddress =
      Multiaddress::create("/ip4/192.168.0.1").value();

  boost::asio::io_context io_context_;
};

/**
 * @given host builder
 * @when building a host with no parameters
 * @then build does not fail
 */
TEST_F(HostBuilderTest, AllDefaults) {
  HostBuilder{io_context_}.build();
}
