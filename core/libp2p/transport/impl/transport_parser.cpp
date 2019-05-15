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
    case E::PROTOCOLS_UNSUPPORTED:
      return "This protocol is not supported by libp2p transport";
  }
  return "Unknown error";
}

namespace libp2p::transport {

  using boost::lexical_cast;
  using boost::asio::ip::make_address;
  using multi::Protocol;

  // clang-format off
  const std::list<std::list<Protocol::Code>> TransportParser::kSupportedProtocols {
    {multi::Protocol::Code::ip4, multi::Protocol::Code::tcp}
  };
  // clang-format on

  outcome::result<TransportParser::ParseResult> TransportParser::parse(
      const multi::Multiaddress &address) {
    auto pvs = address.getProtocolsWithValues();

    int16_t chosen_entry = -1;
    auto entry = std::find_if(
        kSupportedProtocols.begin(), kSupportedProtocols.end(),
        [&pvs](auto const &entry) {
          return std::equal(entry.begin(), entry.end(), pvs.begin(), pvs.end(),
                            [](Protocol::Code entry_proto, auto &pair) {
                              return entry_proto == pair.first.code;
                            });
        });
    if (entry == kSupportedProtocols.end()) {
      return Error::PROTOCOLS_UNSUPPORTED;
    }
    chosen_entry = std::distance(std::begin(kSupportedProtocols), entry);

    switch (chosen_entry) {
      case 0: {
        auto it = pvs.begin();
        auto addr = parseIp(it->second);
        it++;
        auto port = parseTcp(it->second);
        return ParseResult{*entry, std::make_pair(addr, port)};
      }
      default:
        return Error::PROTOCOLS_UNSUPPORTED;
    }
  }

  uint16_t TransportParser::parseTcp(std::string_view value) {
    return lexical_cast<uint16_t>(value);
  }

  TransportParser::IpAddress TransportParser::parseIp(std::string_view value) {
    return make_address(value);
  }

}  // namespace libp2p::transport
