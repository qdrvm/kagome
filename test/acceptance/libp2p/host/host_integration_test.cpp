/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/host/basic_host/basic_host.hpp"

#include <chrono>
#include <future>
#include <thread>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <gsl/span>
#include "common/buffer.hpp"
#include "libp2p/injector/host_injector.hpp"
#include "libp2p/injector/network_injector.hpp"
#include "libp2p/protocol/echo.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using namespace libp2p;
using namespace injector;

using ::testing::_;
using ::testing::Return;
using ::testing::ReturnRef;

using std::chrono_literals::operator""ms;
using std::chrono_literals::operator""s;

using connection::Stream;

struct HostIntegrationTest : public ::testing::Test {
  template <class T>
  using sptr = std::shared_ptr<T>;
  template <class T>
  using uptr = std::unique_ptr<T>;
  using host_uptr_t = uptr<Host>;

  auto makeHost() {
    // create hosts with default muxer, transport, security adaptors
    auto ctx = std::make_shared<boost::asio::io_context>(1);
    auto injector =
        makeHostInjector(boost::di::bind<boost::asio::io_context>.to(ctx),
                         boost::di::bind<muxer::MuxedConnectionConfig>.to(
                             muxer::MuxedConnectionConfig::makeDefault()));

    sptr<Host> host = injector.create<std::shared_ptr<Host>>();

    return std::make_pair(ctx, host);
  }

  /// VARS
  multi::Multiaddress addr1 = "/ip4/127.0.0.1/tcp/40510"_multiaddr;
  multi::Multiaddress addr2 = "/ip4/127.0.0.1/tcp/40511"_multiaddr;

  std::vector<multi::Multiaddress> mas{addr1, addr2};
};

struct HIHelper {
  virtual ~HIHelper() = default;
  virtual void callback1() = 0;
  virtual void callback2() = 0;
  virtual void callback3() = 0;
  virtual void callback4() = 0;
};

struct HIHelperMock : public HIHelper {
  ~HIHelperMock() override = default;
  MOCK_METHOD0(callback1, void(void));
  MOCK_METHOD0(callback2, void(void));
  MOCK_METHOD0(callback3, void(void));
  MOCK_METHOD0(callback4, void(void));
};

TEST_F(HostIntegrationTest, PreliminaryTest) {
  std::string msg1 = "hello 1";
  std::string msg2 = "hello 2";

  HIHelperMock helper;

  EXPECT_CALL(helper, callback1()).WillOnce(Return());
  EXPECT_CALL(helper, callback2()).WillRepeatedly(Return());
  EXPECT_CALL(helper, callback3()).WillOnce(Return());
  EXPECT_CALL(helper, callback4()).WillOnce(Return());

  std::promise<peer::PeerInfo> pi_promise;
  std::shared_future<peer::PeerInfo> pi_future(pi_promise.get_future());

  auto timeout = 10000ms;

  std::thread c1([this, timeout, &helper, &pi_promise]() mutable {
    auto [ctx, host] = makeHost();
    auto echo = std::make_shared<libp2p::protocol::Echo>();
    host->setProtocolHandler(echo->getProtocolId(),
                             [&](std::shared_ptr<connection::Stream> result) {
                               std::cout << "connection accepted" << std::endl;
                               helper.callback3();
                               echo->handle(result);
                             });
    EXPECT_OUTCOME_TRUE_1(host->listen(addr1))
    pi_promise.set_value(host->getPeerInfo());
    host->start();
    helper.callback4();
    ctx->run_for(timeout);
    ctx->run();
  });

  std::thread c2([this, timeout, &helper, pi_future]() mutable {
    pi_future.wait_for(timeout);
    if (!pi_future.valid()) {
      FAIL();
      return;
    }

    auto pi = pi_future.get();

    libp2p::protocol::Echo echo{};
    auto [ctx, host] = makeHost();
    kagome::common::Buffer msg = "helloooo"_buf;

    host->newStream(
        pi_future.get(), echo.getProtocolId(),
        [&](outcome::result<sptr<Stream>> stream_result) {
          std::cout << "server1 on new stream" << std::endl;

          EXPECT_OUTCOME_TRUE(stream, stream_result)
          helper.callback1();
          gsl::span<uint8_t> wspan = msg.toVector();

          stream->write(wspan, wspan.size(),
                        [&](outcome::result<size_t> written_bytes) {
                          std::cout << "stream write success" << std::endl;
                        });
//          auto client = echo.createClient(stream);
//          client->sendAnd("helloooooo", [](outcome::result<std::string> res) {
//            std::cout << "here we are" << std::endl;
//            EXPECT_OUTCOME_TRUE(str, res)
//            std::cout << "received: " << str << std::endl;
//          });
        });
    ctx->run_for(timeout);
    ctx->run();
  });

  c2.join();
  c1.join();
}
