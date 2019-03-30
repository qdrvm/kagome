/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "testutil/outcome.hpp"

#include "libp2p/transport/tcp.hpp"

using namespace libp2p::transport;
using namespace libp2p::multi;

TEST(TCP, MultipleListenersCanNotWorkOnSameMultiaddr) {
  boost::asio::io_context context;
  auto transport = std::make_unique<TcpTransport>(context);

  auto listener1 = transport->createListener([](auto &&c) {
    ASSERT_TRUE(c) << "listener 1 is nullptr";
    std::cout << "new connection - listener 1\n";
  });
  ASSERT_TRUE(listener1);

  auto listener2 = transport->createListener([](auto &&c) {
    ASSERT_TRUE(c) << "listener 2 is nullptr";
    std::cout << "new connection - listener 2\n";
  });
  ASSERT_TRUE(listener2);

  EXPECT_OUTCOME_TRUE(ma, Multiaddress::create("/ip4/127.0.0.1/tcp/40003"))
  ASSERT_TRUE(listener1->listen(ma));
  ASSERT_TRUE(listener2->listen(ma));

  EXPECT_OUTCOME_TRUE(conn, transport->dial(ma));

  context.run_one();
}
