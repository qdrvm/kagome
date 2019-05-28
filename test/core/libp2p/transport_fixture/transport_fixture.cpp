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
  using transport::TransportListener;

  TransportFixture::TransportFixture()
      : context_{1}, executor_{context_.get_executor()} {}

  void TransportFixture::SetUp() {
    transport_ = std::make_shared<TcpTransport<decltype(executor_)>>(executor_);
    ASSERT_TRUE(transport_) << "cannot create transport1";

    // create multiaddress, from which we are going to connect
    auto ma = "/ip4/127.0.0.1/tcp/40009"_multiaddr;
    multiaddress_ = std::make_shared<Multiaddress>(std::move(ma));
  }

  void TransportFixture::server(
      const TransportListener::HandlerFunc &on_success,
      const TransportListener::ErrorFunc &on_error) {
    transport_listener_ = transport_->createListener(on_success, on_error);
    EXPECT_TRUE(transport_listener_->listen(*multiaddress_))
        << "is port 40009 busy?";
  }

  void TransportFixture::client(
      const TransportListener::HandlerFunc &on_success,
      const TransportListener::ErrorFunc &on_error) {
    transport_->dial(*multiaddress_, on_success, on_error);
  }

  void TransportFixture::launchContext() {
    using std::chrono_literals::operator""ms;
    context_.run_for(50ms);
  }

}  // namespace libp2p::testing
