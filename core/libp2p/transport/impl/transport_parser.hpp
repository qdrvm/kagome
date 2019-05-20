/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRANSPORT_PARSER_HPP
#define KAGOME_TRANSPORT_PARSER_HPP

#include <boost/variant.hpp>
#include <boost/asio/ip/address.hpp>
#include "libp2p/multi/multiaddress.hpp"

namespace libp2p::transport {

  /**
   * Extracts information stored in the provided multiaddress if the protocol
   * stack is supported by transport implementation
   */
  class TransportParser {
   public:
    using IpAddress = boost::asio::ip::address;
    using AddressData = boost::variant<std::pair<IpAddress, uint16_t>>;

    enum class Error { PROTOCOLS_UNSUPPORTED = 1, INVALID_ADDR_VALUE };

    struct ParseResult {
      /// @brief protocols from the list of supported protocols that were stored
      /// in the parsed multiaddress
      const std::vector<multi::Protocol::Code> &chosen_protos_;
      AddressData data_;
    };

    /**
     * @brief Protocols supported by transport
     * A multiaddress passed to a transport instance must contain protocols
     * corresponding to some entry in supported protocol list
     * @example /ip4/127.0.0.1/tcp/1337 is a supported protocol sequence, as
     * entry {ip4, tcp} is in the list, whereas
     * /ip4/127.0.0.1/ipfs/QmcgpsyWgH8Y8ajJz1Cu is not
     */
    static const std::vector<std::vector<multi::Protocol::Code>>
        kSupportedProtocols;

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

   private:
    static outcome::result<uint16_t> parseTcp(std::string_view value);
    static outcome::result<IpAddress> parseIp(std::string_view value);
  };

}  // namespace libp2p::transport

OUTCOME_HPP_DECLARE_ERROR(libp2p::transport, TransportParser::Error);

#endif  // KAGOME_TRANSPORT_PARSER_HPP
