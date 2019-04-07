/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <chrono>

#include <gtest/gtest.h>
#include <boost/asio/io_context.hpp>

#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/transport/impl/transport_impl.hpp"
#include "testutil/outcome.hpp"
#include "libp2p/transport/asio/asio_app.hpp"

using namespace libp2p::transport;
using namespace libp2p::multi;
using kagome::common::Buffer;
using std::chrono_literals::operator""ms;

/**
 * @given boost asio context, initialized transport and single listener
 * @when create listener, assign callbacks
 * @then no errors
 * @when sync write methods executed
 * @then sync read methods executed with correct data
 *
 */
TEST(TCP, Integration) {
  auto buf = Buffer{1, 3, 3, 7};
  bool onStartListening = false;
  bool createListener = false;
  bool onNewConnection = false;
  bool onClose = false;
  bool onError = false;

  boost::asio::io_context context;
  auto transport = std::make_unique<TransportImpl>(context);
  ASSERT_TRUE(transport) << "transport is nullptr";

  auto listener = transport->createListener([&](std::shared_ptr<Connection> c) {
    ASSERT_FALSE(c->isClosed());
    ASSERT_TRUE(c) << "createListener: "
                   << "connection is nullptr";

    EXPECT_OUTCOME_TRUE(addr, c->getRemoteMultiaddr());

    std::cout << "Got new connection: " << addr.getStringAddress() << '\n';
    createListener = true;

    // blocks until 1 byte is available
    EXPECT_OUTCOME_TRUE(v1, c->read(1));
    ASSERT_EQ(v1.toHex(), "01");

    // blocks until 1..7 bytes available. in our case 7 bytes available, so read
    // everything.
    EXPECT_OUTCOME_TRUE(v2, c->readSome(7));
    ASSERT_EQ(v2.toHex(), "02030405060708");

    // does not block. reads everything that was sent to our socket.
    c->readAsync([&buf](outcome::result<Buffer> r) {
      ASSERT_TRUE(r);
      auto &&val = r.value();
      ASSERT_EQ(val, buf);
    });
  });

  ASSERT_TRUE(listener);

  listener->onStartListening([&](const Multiaddress &m) {
    std::cout << "onStartListening on " << m.getStringAddress() << '\n';
    onStartListening = true;
  });

  listener->onNewConnection([&](std::shared_ptr<Connection> c) -> void {
    EXPECT_OUTCOME_TRUE(addr, c->getRemoteMultiaddr());
    std::cout << "onNewConnection: " << addr.getStringAddress() << '\n';
    onNewConnection = true;
  });

  listener->onClose([&](Multiaddress ma) {
    std::cout << "onClose " << ma.getStringAddress() << "\n";
    onClose = true;
  });

  listener->onError([&](std::error_code e) { FAIL() << e.message(); });

  ASSERT_TRUE(listener->isClosed()) << "listener is not closed";

  EXPECT_OUTCOME_TRUE(ma, Multiaddress::create("/ip4/127.0.0.1/tcp/40009"));
  EXPECT_TRUE(listener->listen(ma));
  auto listening_on = listener->getAddresses();
  ASSERT_FALSE(listening_on.empty());
  ASSERT_EQ(listening_on.size(), 1);
  auto listener_ma = listening_on[0];

  EXPECT_OUTCOME_TRUE(conn, transport->dial(listener_ma));
  ASSERT_EQ(listener_ma, ma);

  // read
  ASSERT_TRUE(conn->writeSome(Buffer{1, 2, 3, 4}));

  // readSome
  ASSERT_TRUE(conn->write(Buffer{5, 6, 7, 8}));

  conn->writeAsync(buf, [&buf](std::error_code ec, size_t written) {
    ASSERT_EQ(written, buf.size());
    ASSERT_TRUE(!ec);
  });

  context.run_for(100ms);

  ASSERT_EQ(listener->getAddresses(), std::vector<Multiaddress>{ma});

  ASSERT_TRUE(listener->close());

  ASSERT_TRUE(onStartListening);
  ASSERT_TRUE(onNewConnection);
  ASSERT_TRUE(createListener);
  ASSERT_TRUE(onClose);
  ASSERT_FALSE(onError);
}
