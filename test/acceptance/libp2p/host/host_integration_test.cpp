/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/host/basic_host/basic_host.hpp"

#include <chrono>
#include <thread>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <boost/fusion/include/pair.hpp>
#include "libp2p/injector/host_injector.hpp"
#include "libp2p/injector/network_injector.hpp"
#include "libp2p/protocol/echo.hpp"
#include "libp2p/protocol/ping.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using namespace libp2p;
using namespace injector;

using ::testing::_;
using ::testing::Return;
using ::testing::ReturnRef;

using std::chrono_literals::operator""ms;

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
        makeHostInjector(boost::di::bind<boost::asio::io_context>.to(ctx));

    sptr<Host> host = injector.create<std::shared_ptr<Host>>();

    // make protocol

    //    host->getNetwork().getListener().setProtocolHandler(
    //        "/ping/",
    //        [](outcome::result<sptr<connection::Stream>> result) { return; });

    return std::make_pair(ctx, host);
  }

  /// VARS
  std::vector<peer::PeerId> peerIds = {"1"_peerid, "2"_peerid};

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
  peer::PeerInfo pinfo1{peerIds[0], {addr1}};
  //  peer::PeerInfo pinfo2{peerIds[1], {mas[1]}};

  std::string msg1 = "hello 1";
  std::string msg2 = "hello 2";

  HIHelperMock helper;

  EXPECT_CALL(helper, callback1()).WillOnce(Return());
  EXPECT_CALL(helper, callback2()).WillRepeatedly(Return());
  EXPECT_CALL(helper, callback3()).WillOnce(Return());
  EXPECT_CALL(helper, callback4()).WillOnce(Return());
  libp2p::protocol::Echo echo{};

  auto [ctx1, host1] = makeHost();
  host1->setProtocolHandler(echo.getProtocolId(), [&](auto &&result) {
    std::cout << "connection accepted" << std::endl;
    helper.callback3();
    //      echo.handle(result);
  });
  auto pi = host1->getPeerInfo();
  pi.addresses.push_back(addr1);

  auto timeout = 100000ms;

  std::thread c1([this, ctx = ctx1, host = host1, timeout, &helper]() mutable {
    EXPECT_OUTCOME_TRUE_1(host->listen(addr1))
    host->start();
    helper.callback4();
    ctx->run();
    ctx->run_for(timeout);
  });

  std::thread c2([this, timeout, pinfo1, &helper, pi]() mutable {
    std::this_thread::sleep_for(2000ms);
    libp2p::protocol::Echo echo{};
    auto [ctx, host] = makeHost();

    host->newStream(pi, echo.getProtocolId(),
                    [&](outcome::result<sptr<Stream>> stream_result) {
                      std::cout << "server1 on new stream" << std::endl;

                      EXPECT_OUTCOME_TRUE(stream, stream_result)
                      helper.callback1();
                      auto client = echo.createClient(stream);
                      client->sendAnd(
                          "helloooooo", [](outcome::result<std::string> res) {
                            std::cout << "here we are" << std::endl;
                            EXPECT_OUTCOME_TRUE(str, res)
                            std::cout << "received: " << str << std::endl;
                          });
                    });
    ctx->run_for(timeout);
  });

  c1.join();
  c2.join();
}
