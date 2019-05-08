/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "core/libp2p/transport_fixture/transport_fixture.hpp"

#include "libp2p/transport/impl/transport_impl.hpp"

namespace libp2p::testing {
  using kagome::common::Buffer;
  using libp2p::multi::Multiaddress;
  using libp2p::transport::Connection;
  using libp2p::transport::TransportImpl;

  using std::chrono_literals::operator""ms;

  void TransportFixture::SetUp() {
    init();
  }

  void TransportFixture::init() {
    // create transport
    transport_ = std::make_unique<TransportImpl>(context_);
    ASSERT_TRUE(transport_) << "cannot create transport";

    // create multiaddress, from which we are going to connect
    EXPECT_OUTCOME_TRUE(ma, Multiaddress::create("/ip4/127.0.0.1/tcp/40009"))
    multiaddress_ = std::make_shared<Multiaddress>(std::move(ma));
  }

  void TransportFixture::defaultDial() {
    // make the listener listen to the multiaddress
    EXPECT_TRUE(transport_listener_->listen(*multiaddress_))
        << "is port 40009 busy?";

    // dial to our "server", getting a connection
    EXPECT_OUTCOME_TRUE(conn, transport_->dial(*multiaddress_))
    connection_ = std::move(conn);
  }

  void TransportFixture::launchContext() {
    context_.run_for(50ms);
  }

}  // namespace libp2p::testing
