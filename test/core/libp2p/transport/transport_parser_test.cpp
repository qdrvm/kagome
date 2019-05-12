/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/transport/impl/transport_parser.hpp"
#include <gtest/gtest.h>
#include <outcome/outcome.hpp>
#include <testutil/outcome.hpp>

using libp2p::multi::Multiaddress;
using libp2p::transport::SupportedProtocol;
using libp2p::transport::TransportParser;

using ::testing::Test;

struct TransportParserTest : public Test {
  TransportParserTest()
      : addr_{Multiaddress::create("/ip4/127.0.0.1/tcp/5050").value()},
        unsupported_addr_{
            Multiaddress::create("/ipfs/QmcgpsyWgH8Y8ajJz1Cu72KnS5uo2Aa2LpzU7kinSupNKC/tcp/1234").value()} {}

  const Multiaddress addr_;
  const Multiaddress unsupported_addr_;
};

/**
 * @given Transport parser and a multiaddress
 * @when parse the address
 * @then the supported protocol field in the result has the value corresponding
 * to the content of the multiaddress
 */
TEST_F(TransportParserTest, Parse) {
  auto r = TransportParser::parse(addr_);
  ASSERT_EQ(r.value().proto_, SupportedProtocol::IpTcp);

  r = TransportParser::parse(unsupported_addr_);
  EXPECT_OUTCOME_FALSE_1(r);
}

/**
 * @given Transport parser and a multiaddress
 * @when parse the address
 * @then the parse result variant contains information corresponding to the
 * content of the multiaddress
 */
TEST_F(TransportParserTest, Visit) {
  auto r = TransportParser::parse(addr_);

  class Visitor {
   public:
    bool operator()(
        const std::pair<TransportParser::IpAddress, uint16_t> &ip_tcp) {
      return ip_tcp.first.to_string() == "127.0.0.1" && ip_tcp.second == 5050;
    }

    bool operator()(int i) {
      return false;
    }
  } visitor;

  ASSERT_TRUE(std::visit(visitor, r.value().data_));
}
