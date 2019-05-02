/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol_muxer/multiselect.hpp"

#include <gtest/gtest.h>
#include <testutil/outcome.hpp>
#include "core/libp2p/transport_fixture/transport_fixture.hpp"

using libp2p::peer::Protocol;
using libp2p::protocol_muxer::Multiselect;
using libp2p::protocol_muxer::ProtocolMuxer;

class MultiselectTest : public libp2p::testing::TransportFixture {
 public:
  const Protocol kDefaultEncryptionProtocol = "/plaintext/1.0.0";
  const Protocol kDefaultMultiplexerProtocol = "/mplex/6.7.0";

  Multiselect multiselect_;
};

/**
 * @given stream, over which we want to negotiate, @and multiselect instance
 * over that stream @and two protocols, supported by both sides
 * @when negotiating about the protocols
 * @then the two common protocols are selected
 */
TEST_F(MultiselectTest, NegotiateSuccess) {
//  multiselect_.addEncryptionProtocol(kDefaultEncryptionProtocol);
//  multiselect_.addMultiplexerProtocol(kDefaultMultiplexerProtocol);
//
//  auto stream = getNewStream();
//  multiselect_.negotiateClient(
//      *stream,
//      [this](
//          outcome::result<ProtocolMuxer::NegotiatedProtocols> protocols_res) {
//        EXPECT_OUTCOME_TRUE(protocols, protocols_res)
//        EXPECT_EQ(protocols.encryption_protocol_, kDefaultEncryptionProtocol);
//        EXPECT_EQ(protocols.multiplexer_protocol_, kDefaultMultiplexerProtocol);
//      });
}

TEST_F(MultiselectTest, NegotiateFailure) {}

TEST_F(MultiselectTest, NoProtocols) {}
