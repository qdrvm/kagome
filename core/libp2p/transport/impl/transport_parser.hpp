/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRANSPORT_PARSER_HPP
#define KAGOME_TRANSPORT_PARSER_HPP

#include <variant>

#include <boost/asio/ip/address.hpp>
#include "libp2p/multi/multiaddress.hpp"

namespace libp2p::transport {

  /**
   * Protocols supported by transport
   */
  enum class SupportedProtocol : size_t { IpTcp = 0 };

  /**
   * Extracts information stored in the provided multiaddress if the protocol
   * stack is supported by transport implementation
   */
  class TransportParser {
   public:
    using IpAddress = boost::asio::ip::address;
    using AddressData = std::variant<std::pair<IpAddress, uint16_t>>;

    enum class Error { PROTOCOL_UNSUPPORTED = 1 };

    struct ParseResult {
      SupportedProtocol proto_;
      AddressData data_;
    };

    /**
     * Parses a multiaddress, if it contains supported protocols, and extracts
     * data from it
     * @param address multiaddress that stores information about address to be
     * used by transport. Must contain supported protocols
     * @return a structure containing the information of what protocol stack the
     * multiaddress contains and a variant with the data extracted from it
     */
    static outcome::result<ParseResult> parse(
        const multi::Multiaddress &address);
  };

}  // namespace libp2p::transport

OUTCOME_HPP_DECLARE_ERROR(libp2p::transport, TransportParser::Error);

#endif  // KAGOME_TRANSPORT_PARSER_HPP
