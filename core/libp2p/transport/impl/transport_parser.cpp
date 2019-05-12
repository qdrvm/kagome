/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/transport/impl/transport_parser.hpp"

#include <boost/asio/ip/address.hpp>
#include <boost/lexical_cast.hpp>

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::transport, TransportParser::Error, e) {
  using E = libp2p::transport::TransportParser::Error;
  switch (e) {
    case E::PROTOCOL_UNSUPPORTED:
      return "This protocol is not supported by libp2p transport";
  }
  return "Unknown error";
}

namespace libp2p::transport {

  using boost::lexical_cast;
  using boost::asio::ip::make_address;
  using multi::Protocol;

  outcome::result<TransportParser::ParseResult> TransportParser::parse(
      const multi::Multiaddress &address) {
    ParseResult res;

    auto pvs = address.getProtocolsWithValues();

    auto current = pvs.begin();
    if (current->first.code == Protocol::Code::ip4) {
      auto addr = make_address(current->second);
      current++;
      if (current->first.code == Protocol::Code::tcp) {
        auto port = lexical_cast<uint16_t>(current->second);
        return ParseResult {SupportedProtocol::IpTcp, std::make_pair(addr, port)};
      }
      return Error::PROTOCOL_UNSUPPORTED;
    }

    return Error::PROTOCOL_UNSUPPORTED;
  }

}  // namespace libp2p::transport
