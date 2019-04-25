/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PROTOCOLLIST_HPP
#define KAGOME_PROTOCOLLIST_HPP

#include <array>
#include <functional>
#include <map>
#include <string_view>

namespace libp2p::multi {

  /**
   * Contains some data about a network protocol, e. g. its name and code
   */
  struct Protocol {
   public:
    /**
     * Denotes that the size of the protocol is variable
     */
    static const int kVarLen = -1;

    enum class Code : std::size_t {
      ip4 = 4,
      tcp = 6,
      udp = 273,
      dccp = 33,
      ip6 = 41,
      ip6zone = 42,
      dns = 53,
      dns4 = 54,
      dns6 = 55,
      dnsaddr = 56,
      sctp = 132,
      udt = 301,
      utp = 302,
      unix = 400,
      ipfs = 421,
      onion = 444,
      onion3 = 445,
      garlic64 = 446,
      quic = 460,
      http = 480,
      https = 443,
      ws = 477,
      wss = 478,
      p2p_websocket_star = 479,
      p2p_stardust = 277,
      p2p_webrtc_star = 275,
      p2p_webrtc_direct = 276,
      p2p_circuit = 290,
    };

    constexpr bool operator==(const Protocol &p) const {
      return code == p.code;
    }

    const Code code;
    const ssize_t size;
    const std::string_view name;
  };

  /**
   * Contains a list of protocols and some accessor methods for it
   */
  class ProtocolList {
   public:
    /**
     * The total number of known protocols
     */
    static const std::size_t kProtocolsNum = 28;

    /**
     * Returns a protocol with the corresponding name if it exists, or nullptr
     * otherwise
     */
    static constexpr auto get(std::string_view name) -> Protocol const * {
      for (Protocol const &protocol : protocols_) {
        if (protocol.name == name) {
          return &protocol;
        }
      }
      return nullptr;
    }

    /**
     * Returns a protocol with the corresponding code if it exists, or nullptr
     * otherwise
     */
    static constexpr auto get(Protocol::Code code) -> Protocol const * {
      for (Protocol const &protocol : protocols_) {
        if (protocol.code == code) {
          return &protocol;
        }
      }
      return nullptr;
    }

    /**
     * Return the list of all known protocols
     * @return
     */
    static constexpr auto getProtocols()
        -> const std::array<Protocol, kProtocolsNum> & {
      return protocols_;
    }

   private:
    /**
     * The list of known protocols
     */
    static constexpr const std::array<Protocol, kProtocolsNum> protocols_ = {
        Protocol{Protocol::Code::ip4, 32, "ip4"},
        {Protocol::Code::tcp, 16, "tcp"},
        {Protocol::Code::udp, 16, "udp"},
        {Protocol::Code::dccp, 16, "dccp"},
        {Protocol::Code::ip6, 128, "ip6"},
        {Protocol::Code::ip6zone, Protocol::kVarLen, "ip6zone"},
        {Protocol::Code::dns, Protocol::kVarLen, "dns"},
        {Protocol::Code::dns4, Protocol::kVarLen, "dns64"},
        {Protocol::Code::dns6, Protocol::kVarLen, "dns6"},
        {Protocol::Code::dnsaddr, Protocol::kVarLen, "dnsaddr"},
        {Protocol::Code::sctp, 16, "sctp"},
        {Protocol::Code::udt, 0, "udt"},
        {Protocol::Code::utp, 0, "utp"},
        {Protocol::Code::unix, Protocol::kVarLen, "unix"},
        {Protocol::Code::ipfs, Protocol::kVarLen, "ipfs"},
        {Protocol::Code::onion, 96, "onion"},
        {Protocol::Code::onion3, 296, "onion3"},
        {Protocol::Code::garlic64, Protocol::kVarLen, "garlic64"},
        {Protocol::Code::quic, 0, "quic"},
        {Protocol::Code::http, 0, "http"},
        {Protocol::Code::https, 0, "https"},
        {Protocol::Code::ws, 0, "ws"},
        {Protocol::Code::wss, 0, "wss"},
        {Protocol::Code::p2p_websocket_star, 0, "p2p-websocket-star"},
        {Protocol::Code::p2p_stardust, 0, "p2p-stardust"},
        {Protocol::Code::p2p_webrtc_star, 0, "p2p-webrtc-star"},
        {Protocol::Code::p2p_webrtc_direct, 0, "p2p-webrtc-direct"},
        {Protocol::Code::p2p_circuit, 0, "p2p-circuit"},
    };
  };

}  // namespace libp2p::multi
#endif  // KAGOME_PROTOCOLLIST_HPP
