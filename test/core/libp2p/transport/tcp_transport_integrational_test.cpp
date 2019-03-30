/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/transport/tcp.hpp"
#include "testutil/outcome.hpp"

using namespace libp2p::transport;
using namespace libp2p::multi;
using namespace boost;
using kagome::common::Buffer;

/**
 * @given boost asio context, initialized transport and single listtener
 * @when create listener, assign callbacks
 * @then no errors
 * @when sync write methods executed
 * @then sync read methods executed with correct data
 *
 */
TEST(TCP, Integration) {
  bool onStartListening = false;
  bool createListener = false;
  bool onNewConnection = false;
  bool onClose = false;
  bool onError = false;

  asio::io_context context;
  auto transport = std::make_unique<TcpTransport>(context);
  ASSERT_TRUE(transport);

  auto listener = transport->createListener([&](std::shared_ptr<Connection> c) {
    std::cout << "Got new connection\n";
    createListener = true;

    // read exactly 4 bytes
    EXPECT_OUTCOME_TRUE(v1, c->read(4));
    ASSERT_EQ(v1.toHex(), "01020304");

    // read 1..4 bytes
    EXPECT_OUTCOME_TRUE(v2, c->readSome(4));
    ASSERT_EQ(v2.toHex(), "01");
  });

  ASSERT_TRUE(listener);

  listener->onStartListening([&](const Multiaddress &m) {
    std::cout << "onStartListening on " << m.getStringAddress() << '\n';
    onStartListening = true;
  });

  listener->onNewConnection([&](std::shared_ptr<Connection> c) -> void {
    std::cout << "onNewConnection\n";
    onNewConnection = true;
  });

  listener->onClose([&]() {
    std::cout << "onClose\n";
    onClose = true;
  });

  listener->onError([&](std::error_code e) { FAIL() << e.message(); });

  EXPECT_OUTCOME_TRUE(ma, Multiaddress::create("/ip4/127.0.0.1/tcp/40002"));
  EXPECT_TRUE(listener->listen(ma));
  EXPECT_OUTCOME_TRUE(conn, transport->dial(ma));

  // read
  auto ec1 = conn->writeSome(Buffer{1, 2, 3, 4});
  ASSERT_TRUE(!ec1) << ec1.message();

  // readSome
  auto ec2 = conn->writeSome(Buffer{1});
  ASSERT_TRUE(!ec2) << ec2.message();

  auto ec3 = conn->close();
  ASSERT_TRUE(!ec3) << ec3.message();

  context.run_one();  // run all handlers once

  ASSERT_EQ(listener->getAddresses(), std::vector<Multiaddress>{ma});

  ASSERT_TRUE(!transport->close()) << "failed during closing";

  ASSERT_TRUE(onStartListening);
  ASSERT_TRUE(onNewConnection);
  ASSERT_TRUE(createListener);
  ASSERT_TRUE(onClose);
  ASSERT_FALSE(onError);
}
