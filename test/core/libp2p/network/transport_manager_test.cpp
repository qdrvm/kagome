/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/network/impl/transport_manager_impl.hpp"

#include <gtest/gtest.h>
#include "mock/libp2p/transport/transport_mock.hpp"
#include "testutil/literals.hpp"

using namespace libp2p::network;
using namespace libp2p::multi;

class TransportManagerTest : public testing::Test {
 protected:
  const Multiaddress kDefaultMultiaddress =
      "/ip4/192.168.0.1/tcp/228"_multiaddr;
};

TEST_F(TransportManagerTest, CreateEmpty) {
  TransportManagerImpl manager{};

  ASSERT_TRUE(manager.getAll().empty());
}

TEST_F(TransportManagerTest, CreateFromVector) {}

TEST_F(TransportManagerTest, AddTransports) {}

TEST_F(TransportManagerTest, Clear) {}

TEST_F(TransportManagerTest, FindBestSuccess) {}

TEST_F(TransportManagerTest, FindBestFailure) {}
