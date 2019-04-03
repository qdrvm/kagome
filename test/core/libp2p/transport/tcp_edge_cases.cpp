/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <chrono>
#include <iostream>
#include <string>
#include <system_error>

#include <gtest/gtest.h>

#include "libp2p/transport/tcp.hpp"
#include "testutil/outcome.hpp"

using namespace libp2p::transport;
using namespace libp2p::multi;
using std::chrono_literals::operator""s;
using std::chrono_literals::operator""ms;
using kagome::common::Buffer;

/**
 * @given two listeners
 * @when bound on the same multiaddress
 * @then get error
 */
TEST(TCP, TwoListenersCantBindOnSamePort) {
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
  auto r = listener2->listen(ma);
  ASSERT_EQ(r.error().value(), (int)std::errc::address_in_use);
}

/**
 * @given Echo server with single listener
 * @when parallel clients connect and send random message
 * @then each client is expected to receive sent message
 */
TEST(TCP, SingleListenerCanAcceptManyClients) {
  const int kClients = 2;
  const int kSize = 1500;
  const int kRetries = 10;

  size_t counter = 0;  // number of answers
  boost::asio::io_context context;
  auto transport = std::make_unique<TcpTransport>(context);
  auto listener = transport->createListener([&counter](std::shared_ptr<Connection> c) {
    c->readAsync([c, &counter](outcome::result<Buffer> result) {
      EXPECT_OUTCOME_TRUE(data, result);

      // echo once, then close connection
      c->writeAsync(data,
                    [s = data.size(), c, &counter](std::error_code error,
                                                   size_t written) {
                      counter++;
                      ASSERT_FALSE(error);
                      ASSERT_EQ(written, s);
                      ASSERT_TRUE(c->close());
                    });
    });
  });
  ASSERT_TRUE(listener);
  EXPECT_OUTCOME_TRUE(ma, Multiaddress::create("/ip4/127.0.0.1/tcp/40003"))
  ASSERT_TRUE(listener->listen(ma));

  std::vector<std::thread> clients(kClients);
  std::generate(clients.begin(), clients.end(), [&]() {
    return std::thread([&]() {
      for (int i = 0; i < kRetries; i++) {
        srand(i);

        EXPECT_OUTCOME_TRUE(conn, transport->dial(ma));

        Buffer buf(kSize, 0);
        std::generate(buf.begin(), buf.end(), []() { return rand(); });

        // we don't want to block before context.run
        conn->writeAsync(buf, [&](std::error_code ec, size_t written) {
          ASSERT_FALSE(ec);  // no error
          ASSERT_EQ(written, buf.size());
        });

        EXPECT_OUTCOME_TRUE(val, conn->read(buf.size()));
        ASSERT_EQ(buf, val);
      }
    });
  });

  context.run_for(1s);
  std::for_each(clients.begin(), clients.end(),
                [](std::thread &t) { t.join(); });

  ASSERT_EQ(counter, kRetries * kClients)
      << "not all clients' requests were handled";
}

/**
 * @given tcp transport
 * @when dial to non-existent server (listener)
 * @then get connection_refused error
 */
TEST(TCP, DialToNoServer) {
  boost::asio::io_context context;
  auto transport = std::make_unique<TcpTransport>(context);
  EXPECT_OUTCOME_TRUE(ma, Multiaddress::create("/ip4/127.0.0.1/tcp/40003"));

  auto &&res = transport->dial(ma);
  ASSERT_FALSE(res);
  auto &&e = res.error();

  ASSERT_EQ(e.value(), (int)std::errc::connection_refused);
}
