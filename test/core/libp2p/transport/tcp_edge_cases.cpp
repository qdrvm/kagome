#include <utility>

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

#include "libp2p/transport/impl/transport_impl.hpp"
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
  auto transport = std::make_unique<TransportImpl>(context);

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
  const int kClients = 4;
  const int kSize = 1500;
  const int kRetries = 10;
  size_t counter = 0;  // number of answers

  struct Echo : public std::enable_shared_from_this<Echo> {
    void do_read() {
      if (conn->isClosed()) {
        return;
      }

      conn->asyncRead(buf, size,
                      [t = shared_from_this()](auto &&ec, size_t read) {
                        t->do_read_completed(ec, read);
                      });
    }

    void do_read_completed(const boost::system::error_code &ec, size_t read) {
      if (conn->isClosed()) {
        return;
      }

      EXPECT_FALSE(ec);
      EXPECT_EQ(read, size);

      do_write();
    }

    void do_write() {
      if (conn->isClosed()) {
        return;
      }

      conn->asyncWrite(buf, [t = shared_from_this()](auto &&ec, size_t write) {
        t->do_write_completed(ec, write);
      });
    }

    void do_write_completed(const boost::system::error_code &ec, size_t write) {
      if (conn->isClosed()) {
        return;
      }

      EXPECT_FALSE(ec);

      counter++;

      EXPECT_TRUE(conn->close());
    }

    Echo(size_t &c, size_t s, std::shared_ptr<Connection> co)
        : counter(c), size(s), conn(std::move(co)) {}

   private:
    size_t &counter;
    size_t size;
    std::shared_ptr<Connection> conn;
    boost::asio::streambuf buf;
  };

  boost::asio::io_context context;
  auto transport = std::make_unique<TransportImpl>(context);
  auto listener = transport->createListener(
      [&counter, kSize](std::shared_ptr<Connection> c) {
        EXPECT_OUTCOME_TRUE(ma, c->getRemoteMultiaddr());
        std::cout << "recv from " << ma.getStringAddress() << " thread id "
                  << std::this_thread::get_id() << "\n";

        std::shared_ptr<Echo> client(new Echo(counter, kSize, c));
        client->do_read();
      });

  ASSERT_TRUE(listener);
  EXPECT_OUTCOME_TRUE(ma, Multiaddress::create("/ip4/127.0.0.1/tcp/40003"))
  ASSERT_TRUE(listener->listen(ma));

  std::vector<std::thread> clients(kClients);
  std::generate(clients.begin(), clients.end(), [&]() {
    return std::thread([&]() {
      boost::asio::io_context context;

      auto transport = std::make_unique<TransportImpl>(context);
      for (int i = 0; i < kRetries; i++) {
        srand(i);

        EXPECT_OUTCOME_TRUE(conn, transport->dial(ma));

        auto buf = std::make_shared<Buffer>(kSize, 0);
        std::generate(buf->begin(), buf->end(), []() {
          return rand();  // NOLINT
        });

        auto rdbuf = std::make_shared<Buffer>(kSize, 1);
        conn->asyncWrite(
            boost::asio::buffer(buf->toVector(), kSize),
            [buf, kSize, conn, rdbuf](auto &&ec, size_t write) {
              ASSERT_FALSE(ec);
              ASSERT_EQ(kSize, write);

              conn->asyncRead(
                  boost::asio::buffer(rdbuf->toVector(), kSize), kSize,
                  [buf, kSize, write, rdbuf](auto &&ec, size_t read) {
                    ASSERT_FALSE(ec);
                    ASSERT_EQ(read, write);
                    ASSERT_EQ(read, kSize);
                    ASSERT_EQ(*rdbuf, *buf);
                  });
            });
      }
      context.run_for(500ms);
    });
  });

  context.run_for(500ms);
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
  auto transport = std::make_unique<TransportImpl>(context);
  EXPECT_OUTCOME_TRUE(ma, Multiaddress::create("/ip4/127.0.0.1/tcp/40003"));

  auto &&res = transport->dial(ma);
  ASSERT_FALSE(res);
  auto &&e = res.error();

  ASSERT_EQ(e.value(), (int)std::errc::connection_refused);
}
