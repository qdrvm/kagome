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
  void logError(const std::error_code &ec) {
    std::cout << "error(" << ec.value() << "): " << ec.message() << "\n";
  }

  void expectConnectionValid(const std::shared_ptr<CapableConnection> &conn) {
    EXPECT_TRUE(conn);

    EXPECT_OUTCOME_TRUE(mar, conn->remoteMultiaddr())
    EXPECT_OUTCOME_TRUE(mal, conn->localMultiaddr())
    std::ostringstream s;
    s << mar.getStringAddress() << " -> " << mal.getStringAddress();
    std::cout << s.str() << '\n';
  }

  template <typename T, typename R>
  outcome::result<R> _upgrade(T c) {
    R r = std::make_shared<CapableConnBasedOnRawConnMock>(c);
    return outcome::success(r);
  }

  auto makeUpgrader() {
    auto upgrader = std::make_shared<NiceMock<UpgraderMock>>();
    ON_CALL(*upgrader, upgradeToSecure(_))
        .WillByDefault(
            Invoke(_upgrade<Upgrader::RawSPtr, Upgrader::SecureSPtr>));
    ON_CALL(*upgrader, upgradeToMuxed(_))
        .WillByDefault(
            Invoke(_upgrade<Upgrader::SecureSPtr, Upgrader::CapableSPtr>));

    return upgrader;
  }
}  // namespace

/**
 * @given two listeners
 * @when bound on the same multiaddress
 * @then get error
 */
TEST(TCP, TwoListenersCantBindOnSamePort) {
  auto upgrader = makeUpgrader();

  boost::asio::io_context context(1);
  auto e = context.get_executor();
  auto transport =
      std::make_shared<TcpTransport<decltype(e)>>(e, std::move(upgrader));

  auto listener1 = transport->createListener(
      [](auto &&c) {
        EXPECT_TRUE(c) << "listener 1 is nullptr";
        std::cout << "new connection - listener 1\n";
        return outcome::success();
      },
      logError);
  ASSERT_TRUE(listener1);

  auto listener2 = transport->createListener(
      [](auto &&c) {
        EXPECT_TRUE(c) << "listener 2 is nullptr";
        std::cout << "new connection - listener 2\n";
        return outcome::success();
      },
      logError);
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
  constexpr static int kClients = 4;
  constexpr static int kSize = 1500;
  size_t counter = 0;  // number of answers

  auto upgrader = makeUpgrader();
  boost::asio::io_context context(1);
  auto e = context.get_executor();

  auto transport =
      std::make_shared<TcpTransport<decltype(e)>>(e, std::move(upgrader));
  using libp2p::connection::RawConnection;
  auto listener = transport->createListener(
      [&](auto &&conn) {
        expectConnectionValid(conn);
        EXPECT_FALSE(conn->isInitiator());

        EXPECT_OUTCOME_TRUE(buf, conn->readSome(kSize));
        EXPECT_OUTCOME_TRUE(written, conn->write(buf));
        EXPECT_EQ(written, buf.size());
        counter++;

        return outcome::success();
      },
      logError);

  ASSERT_TRUE(listener);
  auto ma = "/ip4/127.0.0.1/tcp/40003"_multiaddr;
  ASSERT_TRUE(listener->listen(ma));

  std::vector<std::thread> clients(kClients);
  std::generate(clients.begin(), clients.end(), [&]() {
    return std::thread([&]() {
      auto upgrader = makeUpgrader();
      boost::asio::io_context context(1);
      auto e = context.get_executor();
      auto transport =
          std::make_shared<TcpTransport<decltype(e)>>(e, std::move(upgrader));

      transport->dial(
          ma,
          [](auto &&conn) {
            expectConnectionValid(conn);

            auto buf = Buffer(kSize, 0);
            std::generate(buf.begin(), buf.end(), []() {
              return rand();  // NOLINT
            });

            EXPECT_TRUE(conn->isInitiator());

            EXPECT_OUTCOME_TRUE(written, conn->write(buf));
            EXPECT_EQ(written, buf.size());
            EXPECT_OUTCOME_TRUE(readback, conn->read(kSize));
            EXPECT_EQ(buf, readback);
            return outcome::success();
          },
          logError);

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
  auto upgrader = makeUpgrader();
  boost::asio::io_context context;
  auto e = context.get_executor();
  auto transport =
      std::make_shared<TcpTransport<decltype(e)>>(e, std::move(upgrader));
  auto ma = "/ip4/127.0.0.1/tcp/40003"_multiaddr;

  transport->dial(
      ma,
      [](auto &&c) {
        ADD_FAILURE() << "should not get here";
        return outcome::success();
      },
      [](auto &&ec) {
        logError(ec);
        ASSERT_TRUE(ec);
        ASSERT_EQ(ec.value(), (int)std::errc::connection_refused);
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
  auto upgrader = makeUpgrader();
  boost::asio::io_context context(1);
  auto e = context.get_executor();

  auto transport =
      std::make_shared<TcpTransport<decltype(e)>>(e, std::move(upgrader));
  auto listener = transport->createListener(
      [&](auto &&conn) {
        expectConnectionValid(conn);
        EXPECT_FALSE(conn->isInitiator());

        EXPECT_OUTCOME_FALSE_2(ec, conn->readSome(100));
        EXPECT_EQ(ec.value(), (int)boost::asio::error::eof) << ec.message();

        return outcome::success();
      },
      [](auto &&ec) { ADD_FAILURE() << "should not get here"; });

  ASSERT_TRUE(listener);
  auto ma = "/ip4/127.0.0.1/tcp/40003"_multiaddr;
  ASSERT_TRUE(listener->listen(ma));

  transport->dial(
      ma,
      [](auto &&conn) {
        expectConnectionValid(conn);
        EXPECT_TRUE(conn->isInitiator());

        EXPECT_TRUE(conn->close());
        return outcome::success();
      },
      logError);

  context.run_for(50ms);
}

/**
 * @given server with one active client
 * @when server closes active connection
 * @then client gets EOF
 */
TEST(TCP, ServerClosesConnection) {
  auto upgrader = makeUpgrader();
  boost::asio::io_context context(1);
  auto e = context.get_executor();

  auto transport =
      std::make_shared<TcpTransport<decltype(e)>>(e, std::move(upgrader));
  auto listener = transport->createListener(
      [&](auto &&conn) {
        expectConnectionValid(conn);
        EXPECT_FALSE(conn->isInitiator());

        EXPECT_OUTCOME_TRUE_1(conn->close());
        return outcome::success();
      },
      [](auto &&ec) { ADD_FAILURE() << "should not get here"; });

  ASSERT_TRUE(listener);
  auto ma = "/ip4/127.0.0.1/tcp/40003"_multiaddr;
  ASSERT_TRUE(listener->listen(ma));

  transport->dial(
      ma,
      [](auto &&conn) {
        expectConnectionValid(conn);
        EXPECT_TRUE(conn->isInitiator());
        EXPECT_OUTCOME_FALSE_2(ec, conn->read(1));
        EXPECT_EQ(ec.value(), (int)boost::asio::error::eof) << ec.message();
        return outcome::success();
      },
      [](auto &&e) { ADD_FAILURE() << "should not get here"; });

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

  auto upgrader = makeUpgrader();
  boost::asio::io_context context(1);
  auto e = context.get_executor();

  auto transport =
      std::make_shared<TcpTransport<decltype(e)>>(e, std::move(upgrader));
  auto listener = transport->createListener(
      [&](auto &&conn) {
        expectConnectionValid(conn);
        EXPECT_FALSE(conn->isInitiator());

        EXPECT_OUTCOME_TRUE(buf, conn->readSome(kSize));
        EXPECT_OUTCOME_TRUE(written, conn->write(buf));
        EXPECT_EQ(written, buf.size());
        counter++;

        return outcome::success();
      },
      logError);

  ASSERT_TRUE(listener);
  auto ma = "/ip4/127.0.0.1/tcp/40003"_multiaddr;
  ASSERT_TRUE(listener->listen(ma));

  transport->dial(
      ma,
      [](auto &&conn) {
        expectConnectionValid(conn);

        auto buf = Buffer(kSize, 0);
        std::generate(buf.begin(), buf.end(), []() {
          return rand();  // NOLINT
        });

        EXPECT_TRUE(conn->isInitiator());

        EXPECT_OUTCOME_TRUE(written, conn->write(buf));
        EXPECT_EQ(written, buf.size());
        EXPECT_OUTCOME_TRUE(readback, conn->read(kSize));
        EXPECT_EQ(buf, readback);
        return outcome::success();
      },
      logError);

  context.run_for(100ms);

  ASSERT_EQ(counter, 1);
}
