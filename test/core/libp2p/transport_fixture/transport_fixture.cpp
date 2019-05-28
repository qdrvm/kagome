/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "core/libp2p/transport_fixture/transport_fixture.hpp"

#include "libp2p/transport/tcp.hpp"
#include "testutil/literals.hpp"

namespace libp2p::testing {
  using kagome::common::Buffer;
  using libp2p::multi::Multiaddress;
  using libp2p::transport::TcpTransport;

  void TransportFixture::SetUp() {
    init();
  }

  void TransportFixture::init() {
    // create transport
    transport_ = std::make_unique<TcpTransport<decltype(executor_)>>(executor_);
    ASSERT_TRUE(transport_) << "cannot create transport";

    // create multiaddress, from which we are going to connect
    auto ma = "/ip4/127.0.0.1/tcp/40009"_multiaddr;
    multiaddress_ = std::make_shared<Multiaddress>(std::move(ma));
  }

  void TransportFixture::defaultDial() {
    // make the listener listen to the multiaddress
    EXPECT_TRUE(transport_listener_->listen(*multiaddress_))
        << "is port 40009 busy?";

    // dial to our "server", getting a connection
    transport_->dial(
        *multiaddress_,
        [this](std::shared_ptr<connection::RawConnection> c) {
          connection_ = std::move(c);
          return outcome::success();
        },
        [](auto &&) { FAIL() << "cannot dial"; });  // NOLINT
  }

  void TransportFixture::launchContext() {
    using std::chrono_literals::operator""ms;
    context_.run_for(50ms);
  }

}  // namespace libp2p::testing
