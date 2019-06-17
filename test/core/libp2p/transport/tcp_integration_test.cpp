/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <system_error>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "libp2p/transport/tcp.hpp"
#include "mock/libp2p/connection/capable_connection_mock.hpp"
#include "mock/libp2p/transport/upgrader_mock.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using namespace libp2p::transport;
using namespace libp2p::multi;
using namespace libp2p::connection;
using std::chrono_literals::operator""s;
using std::chrono_literals::operator""ms;
using kagome::common::Buffer;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;

namespace {
  std::shared_ptr<CapableConnection> expectConnectionValid(
      outcome::result<std::shared_ptr<CapableConnection>> rconn) {
    EXPECT_TRUE(rconn) << rconn.error();
    auto conn = rconn.value();

    EXPECT_OUTCOME_TRUE(mar, conn->remoteMultiaddr())
    EXPECT_OUTCOME_TRUE(mal, conn->localMultiaddr())
    std::ostringstream s;
    s << mar.getStringAddress() << " -> " << mal.getStringAddress();
    std::cout << s.str() << '\n';

    return conn;
  }

  template <typename Conn, typename R, typename Callback>
  void _upgrade(Conn c, Callback cb) {
    R r = std::make_shared<CapableConnBasedOnRawConnMock>(c);
    cb(std::move(r));
  }

  auto makeUpgrader() {
    auto upgrader = std::make_shared<NiceMock<UpgraderMock>>();
    ON_CALL(*upgrader, upgradeToSecure(_, _))
        .WillByDefault(Invoke(_upgrade<Upgrader::RawSPtr, Upgrader::SecureSPtr,
                                       Upgrader::OnSecuredCallbackFunc>));
    ON_CALL(*upgrader, upgradeToMuxed(_, _))
        .WillByDefault(
            Invoke(_upgrade<Upgrader::SecureSPtr, Upgrader::CapableSPtr,
                            Upgrader::OnMuxedCallbackFunc>));

    return upgrader;
  }
}  // namespace

/**
 * @given two listeners
 * @when bound on the same multiaddress
 * @then get error
 */
TEST(TCP, TwoListenersCantBindOnSamePort) {
  boost::asio::io_context context(1);
  auto upgrader = makeUpgrader();
  auto transport = std::make_shared<TcpTransport>(context, std::move(upgrader));
  auto listener1 = transport->createListener([](auto &&c) { EXPECT_TRUE(c); });

  ASSERT_TRUE(listener1);

  auto listener2 = transport->createListener([](auto &&c) { EXPECT_TRUE(c); });
  ASSERT_TRUE(listener2);

  auto ma = "/ip4/127.0.0.1/tcp/40003"_multiaddr;

  std::cout << "listener 1 starting...\n";
  ASSERT_TRUE(listener1->listen(ma));

  std::cout << "listener 2 starting...\n";
  auto r = listener2->listen(ma);
  ASSERT_EQ(r.error().value(), (int)std::errc::address_in_use);

  using std::chrono_literals::operator""ms;
  context.run_for(50ms);
}

/**
 * @given Echo server with single listener
 * @when parallel clients connect and send random message
 * @then each client is expected to receive sent message
 */
TEST(TCP, SingleListenerCanAcceptManyClients) {
  constexpr static int kClients = 2;
  constexpr static int kSize = 1500;
  size_t counter = 0;  // number of answers
  auto ma = "/ip4/127.0.0.1/tcp/40003"_multiaddr;

  boost::asio::io_context context(1);
  auto upgrader = makeUpgrader();
  auto transport = std::make_shared<TcpTransport>(context, std::move(upgrader));
  using libp2p::connection::RawConnection;
  auto listener = transport->createListener([&](auto &&rconn) {
    auto conn = expectConnectionValid(rconn);
    EXPECT_FALSE(conn->isInitiator());

    auto buf = std::make_shared<std::vector<uint8_t>>(kSize, 0);
    conn->readSome(*buf, [&counter, conn, buf](auto &&ec, size_t read) {
      ASSERT_FALSE(ec) << ec.message();

      conn->write(*buf, [&counter, buf](auto &&ec, size_t written) {
        ASSERT_FALSE(ec) << ec.message();
        EXPECT_EQ(written, buf->size());
        counter++;
      });
    });
  });

  ASSERT_TRUE(listener);
  ASSERT_TRUE(listener->listen(ma));

  std::vector<std::thread> clients(kClients);
  std::generate(clients.begin(), clients.end(), [&]() {
    return std::thread([&]() {
      boost::asio::io_context context(1);
      auto upgrader = makeUpgrader();
      auto transport =
          std::make_shared<TcpTransport>(context, std::move(upgrader));
      transport->dial(ma, [](auto &&rconn) {
        auto conn = expectConnectionValid(rconn);

        auto readback = std::make_shared<Buffer>(kSize, 0);
        auto buf = std::make_shared<Buffer>(kSize, 0);
        std::generate(buf->begin(), buf->end(), []() {
          return rand();  // NOLINT
        });

        EXPECT_TRUE(conn->isInitiator());

        conn->write(*buf, [conn, readback, buf](auto &&ec, size_t written) {
          ASSERT_FALSE(ec) << ec.message();
          ASSERT_EQ(written, buf->size());
          conn->read(*readback, [conn, readback, buf](auto &&ec, size_t read) {
            ASSERT_FALSE(ec) << ec.message();
            ASSERT_EQ(read, readback->size());
            ASSERT_EQ(*buf, *readback);
          });
        });
      });

      context.run_for(100ms);
    });
  });

  context.run_for(500ms);
  std::for_each(clients.begin(), clients.end(),
                [](std::thread &t) { t.join(); });

  ASSERT_EQ(counter, kClients) << "not all clients' requests were handled";
}

/**
 * @given tcp transport
 * @when dial to non-existent server (listener)
 * @then get connection_refused error
 */
TEST(TCP, DialToNoServer) {
  boost::asio::io_context context;
  auto upgrader = makeUpgrader();
  auto transport = std::make_shared<TcpTransport>(context, std::move(upgrader));
  auto ma = "/ip4/127.0.0.1/tcp/40003"_multiaddr;

  transport->dial(ma, [](auto &&rc) {
    ASSERT_FALSE(rc);
    ASSERT_EQ(rc.error().value(), (int)std::errc::connection_refused);
  });

  using std::chrono_literals::operator""ms;
  context.run_for(50ms);
}

/**
 * @given server with one active client
 * @when client closes connection
 * @then server gets EOF
 */
TEST(TCP, ClientClosesConnection) {
  boost::asio::io_context context(1);
  auto upgrader = makeUpgrader();
  auto transport = std::make_shared<TcpTransport>(context, std::move(upgrader));
  auto listener = transport->createListener([&](auto &&rconn) {
    auto conn = expectConnectionValid(rconn);
    EXPECT_FALSE(conn->isInitiator());

    auto buf = std::make_shared<std::vector<uint8_t>>(100, 0);
    conn->readSome(*buf, [conn, buf](auto &&ec, size_t read) {
      EXPECT_EQ(ec.value(), (int)boost::asio::error::eof) << ec.message();
    });
  });

  ASSERT_TRUE(listener);
  auto ma = "/ip4/127.0.0.1/tcp/40003"_multiaddr;
  ASSERT_TRUE(listener->listen(ma));

  transport->dial(ma, [](auto &&rconn) {
    auto conn = expectConnectionValid(rconn);
    EXPECT_TRUE(conn->isInitiator());

    EXPECT_TRUE(conn->close());
  });

  context.run_for(50ms);
}

/**
 * @given server with one active client
 * @when server closes active connection
 * @then client gets EOF
 */
TEST(TCP, ServerClosesConnection) {
  boost::asio::io_context context(1);
  auto upgrader = makeUpgrader();
  auto transport = std::make_shared<TcpTransport>(context, std::move(upgrader));
  auto listener = transport->createListener([&](auto &&rconn) {
    auto conn = expectConnectionValid(rconn);
    EXPECT_FALSE(conn->isInitiator());
    EXPECT_OUTCOME_TRUE_1(conn->close());
  });

  ASSERT_TRUE(listener);
  auto ma = "/ip4/127.0.0.1/tcp/40003"_multiaddr;
  ASSERT_TRUE(listener->listen(ma));

  transport->dial(ma, [](auto &&rconn) {
    auto conn = expectConnectionValid(rconn);
    EXPECT_TRUE(conn->isInitiator());
    auto buf = std::make_shared<std::vector<uint8_t>>(100, 0);
    conn->readSome(*buf, [conn, buf](auto &&ec, size_t read) {
      EXPECT_EQ(ec.value(), (int)boost::asio::error::eof) << ec.message();
    });
  });

  context.run_for(50ms);
}

/**
 * @given single thread, single transport on a single default executor
 * @when create server @and dial to this server
 * @then connection successfully established
 */
TEST(TCP, OneTransportServerHandlesManyClients) {
  constexpr static int kSize = 1500;
  size_t counter = 0;  // number of answers

  boost::asio::io_context context(1);
  auto upgrader = makeUpgrader();
  auto transport = std::make_shared<TcpTransport>(context, std::move(upgrader));
  auto listener = transport->createListener([&](auto &&rconn) {
    auto conn = expectConnectionValid(rconn);
    EXPECT_FALSE(conn->isInitiator());

    auto buf = std::make_shared<std::vector<uint8_t>>(kSize, 0);
    conn->readSome(*buf, [&counter, conn, buf](auto &&ec, size_t read) {
      ASSERT_FALSE(ec) << ec.message();

      conn->write(*buf, [&counter, buf](auto &&ec, size_t written) {
        ASSERT_FALSE(ec) << ec.message();
        EXPECT_EQ(written, buf->size());
        counter++;
      });
    });
  });

  ASSERT_TRUE(listener);
  auto ma = "/ip4/127.0.0.1/tcp/40003"_multiaddr;
  ASSERT_TRUE(listener->listen(ma));

  transport->dial(ma, [](auto &&rconn) {
    auto conn = expectConnectionValid(rconn);

    auto readback = std::make_shared<Buffer>(kSize, 0);
    auto buf = std::make_shared<Buffer>(kSize, 0);
    std::generate(buf->begin(), buf->end(), []() {
      return rand();  // NOLINT
    });

    EXPECT_TRUE(conn->isInitiator());

    conn->write(*buf, [conn, readback, buf](auto &&ec, size_t written) {
      ASSERT_FALSE(ec) << ec.message();
      ASSERT_EQ(written, buf->size());
      conn->read(*readback, [conn, readback, buf](auto &&ec, size_t read) {
        ASSERT_FALSE(ec) << ec.message();
        ASSERT_EQ(read, readback->size());
        ASSERT_EQ(*buf, *readback);
      });
    });
  });

  context.run_for(100ms);

  ASSERT_EQ(counter, 1);
}
