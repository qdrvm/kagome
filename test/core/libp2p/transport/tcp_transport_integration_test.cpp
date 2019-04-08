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

using namespace libp2p::transport;
using namespace libp2p::multi;
using kagome::common::Buffer;
using namespace boost::asio;
using std::chrono_literals::operator""ms;

struct Reverse : public std::enable_shared_from_this<Reverse> {
  void do_read() {
    ASSERT_FALSE(conn->isClosed());
    conn->asyncRead(buf, size,
                    [t = shared_from_this()](auto &&ec, size_t read) {
                      t->do_read_completed(ec, read);
                    });
  }

  void do_read_completed(const boost::system::error_code &ec, size_t read) {
    ASSERT_FALSE(conn->isClosed());
    EXPECT_FALSE(ec);
    EXPECT_EQ(read, size);

    do_reverse();
    do_write();
  }

  void do_reverse() {
    ASSERT_FALSE(conn->isClosed());
    std::string in{buffers_begin(buf.data()), buffers_end(buf.data())};
    buf.consume(in.size());
    std::cout << "server> reversing : " << in << "\n";
    std::reverse(in.begin(), in.end());
    std::ostream os(&buf);
    os << in;
    std::cout << "server> reversed  : " << in << "\n";
  }

  void do_write() {
    ASSERT_FALSE(conn->isClosed());
    conn->asyncWrite(buf, [t = shared_from_this()](auto &&ec, size_t write) {
      t->do_write_completed(ec, write);
    });
  }

  void do_write_completed(const boost::system::error_code &ec, size_t write) {
    ASSERT_FALSE(conn->isClosed());
    EXPECT_FALSE(ec);
    EXPECT_TRUE(conn->close());
  }

  Reverse(size_t s, std::shared_ptr<Connection> co)
      : size(s), conn(std::move(co)) {}

 private:
  size_t size;
  std::shared_ptr<Connection> conn;
  boost::asio::streambuf buf;
};

/**
 * @given boost asio context, initialized transport and single listener
 * @when create listener, assign callbacks
 * @then no errors
 * @when sync write methods executed
 * @then sync read methods executed with correct data
 *
 */
TEST(TCP, Integration) {
  const std::string msg = "hello world";

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

    auto r = std::make_shared<Reverse>(msg.size(), c);
    r->do_read();
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
  EXPECT_TRUE(listener->listen(ma)) << "is port 40009 busy?";
  auto listening_on = listener->getAddresses();
  ASSERT_FALSE(listening_on.empty());
  ASSERT_EQ(listening_on.size(), 1);
  auto listener_ma = listening_on[0];

  EXPECT_OUTCOME_TRUE(conn, transport->dial(listener_ma));
  ASSERT_EQ(listener_ma, ma);

  bool asyncWriteExecuted = false;
  bool asyncReadExecuted = false;

  auto rcvbuf = std::make_shared<Buffer>(msg.size(), 0);
  conn->asyncWrite(
      boost::asio::buffer(msg, msg.size()),
      [&asyncReadExecuted, &asyncWriteExecuted, rcvbuf, &msg, conn](
          auto &&ec, size_t write) {
        asyncWriteExecuted = true;
        EXPECT_FALSE(ec);
        EXPECT_EQ(msg.size(), write);

        conn->asyncRead(
            boost::asio::buffer(rcvbuf->toVector()), msg.size(),
            [&asyncReadExecuted, msg, rcvbuf, write](auto &&ec, size_t read) {
              asyncReadExecuted = true;
              EXPECT_FALSE(ec);
              EXPECT_EQ(read, write);
              std::string reversed{rcvbuf->begin(), rcvbuf->end()};
              std::cout << "client> received  : " << reversed << "\n";
              std::string expected{msg.begin(), msg.end()};
              std::reverse(expected.begin(), expected.end());
              EXPECT_EQ(reversed, expected);
            });
      });

  context.run_for(100ms);

  EXPECT_TRUE(asyncReadExecuted);
  EXPECT_TRUE(asyncWriteExecuted);

  ASSERT_EQ(listener->getAddresses(), std::vector<Multiaddress>{ma});

  ASSERT_TRUE(listener->close());

  ASSERT_TRUE(onStartListening);
  ASSERT_TRUE(onNewConnection);
  ASSERT_TRUE(createListener);
  ASSERT_TRUE(onClose);
  ASSERT_FALSE(onError);
}
