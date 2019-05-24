/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <system_error>

#include <gtest/gtest.h>

#include "libp2p/transport/tcp.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using namespace libp2p::transport;
using namespace libp2p::multi;
using namespace libp2p::connection;
using std::chrono_literals::operator""s;
using std::chrono_literals::operator""ms;
using kagome::common::Buffer;

void logError(const std::error_code &ec) {
  std::cout << "error(" << ec.value() << "): " << ec.message() << "\n";
}

void expectConnectionValid(std::shared_ptr<RawConnection> conn){
  EXPECT_TRUE(conn);

  EXPECT_OUTCOME_TRUE(mar, conn->remoteMultiaddr());
  EXPECT_OUTCOME_TRUE(mal, conn->localMultiaddr());
  std::ostringstream s;
  s << mar.getStringAddress() << " -> " << mal.getStringAddress();
  std::string tag = s.str();
  std::cout << tag << '\n';
}

/**
 * @given two listeners
 * @when bound on the same multiaddress
 * @then get error
 */
TEST(TCP, TwoListenersCantBindOnSamePort) {
  boost::asio::io_context context(1);
  auto e = context.get_executor();
  auto transport = std::make_shared<TcpTransport<decltype(e)>>(e);

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
  const int kClients = 4;
  const int kSize = 1500;
  size_t counter = 0;  // number of answers

  boost::asio::io_context context(1);
  auto e = context.get_executor();

  auto transport = std::make_shared<TcpTransport<decltype(e)>>(e);
  using libp2p::connection::RawConnection;
  auto listener = transport->createListener(
      [&](std::shared_ptr<RawConnection> conn) {
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
      boost::asio::io_context context(1);
      auto e = context.get_executor();
      auto transport = std::make_shared<TcpTransport<decltype(e)>>(e);

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
  boost::asio::io_context context;
  auto e = context.get_executor();
  auto transport = std::make_shared<TcpTransport<decltype(e)>>(e);
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
  boost::asio::io_context context(1);
  auto e = context.get_executor();

  auto transport = std::make_shared<TcpTransport<decltype(e)>>(e);
  using libp2p::connection::RawConnection;
  auto listener = transport->createListener(
      [&](std::shared_ptr<RawConnection> conn) {
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
  boost::asio::io_context context(1);
  auto e = context.get_executor();

  auto transport = std::make_shared<TcpTransport<decltype(e)>>(e);
  using libp2p::connection::RawConnection;
  auto listener = transport->createListener(
      [&](std::shared_ptr<RawConnection> conn) {
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
