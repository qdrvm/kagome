/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/multi/converters/converter_utils.hpp"

#include <boost/asio/ip/address_v4.hpp>
#include <outcome/outcome.hpp>
#include "common/buffer.hpp"
#include "common/hexutil.hpp"
#include "libp2p/multi/converters/conversion_error.hpp"
#include "libp2p/multi/converters/ip_v4_converter.hpp"
#include "libp2p/multi/converters/ipfs_converter.hpp"
#include "libp2p/multi/converters/tcp_converter.hpp"
#include "libp2p/multi/converters/udp_converter.hpp"
#include "libp2p/multi/utils/protocol_list.hpp"
#include "libp2p/multi/utils/uvarint.hpp"

using kagome::common::unhex;

namespace libp2p::multi::converters {

  using kagome::common::Buffer;

  outcome::result<Buffer> stringToBytes(std::string_view str) {
    if (str[0] != '/') {
      return ConversionError::kAddressDoesNotBeginWithSlash;
    }
    str.remove_prefix(1);

    std::string processed;

    enum class WordType { Protocol, Address };
    WordType type = WordType::Protocol;

    // Starting to extract words and process them:
    auto delim_pos = str.find("/");
    auto word = str.substr(0, delim_pos);
    str.remove_prefix(delim_pos + 1);

    Protocol const *protx = nullptr;

    while (!word.empty()) {
      if (type == WordType::Protocol) {
        protx = ProtocolList::get(word);
        if (protx != nullptr) {
          processed += UVarint(static_cast<uint64_t>(protx->code)).toHex();
          type = WordType::Address;  // Since the next word will be an address
        } else {
          return ConversionError::kNoSuchProtocol;
        }
      } else {
        auto res = addressToBytes(*protx, word);
        if (!res) {
          return res.error();
        }

        processed += res.value();

        protx = nullptr;  // Since right now it doesn't need that
        // assignment anymore.
        type = WordType::Protocol;  // Since the next word will be an protocol
      }
      delim_pos = str.find("/");
      word = str.substr(0, delim_pos);
      if (delim_pos == std::string_view::npos) {
        str.remove_prefix(str.size());
      } else {
        str.remove_prefix(delim_pos + 1);
      }
    }
    // TODO(Harrm) migrate hexutils to boost::outcome
    auto bytes = kagome::common::unhex(processed);
    return Buffer{bytes.getValue()};
  }

  auto addressToBytes(const Protocol &protocol, std::string_view addr)
      -> outcome::result<std::string> {
    std::string astb__stringy;

    // TODO(Akvinikym) 25.02.19 PRE-49: add more protocols
    switch (protocol.code) {
      case Protocol::Code::ip4:
        return IPv4Converter::addressToBytes(addr);
      case Protocol::Code::tcp:
        return TcpConverter::addressToBytes(addr);
      case Protocol::Code::udp:
        return UdpConverter::addressToBytes(addr);
      case Protocol::Code::ipfs:
        return IpfsConverter::addressToBytes(addr);

      case Protocol::Code::ip6zone:
      case Protocol::Code::dns:
      case Protocol::Code::dns4:
      case Protocol::Code::dns6:
      case Protocol::Code::dnsaddr:
      case Protocol::Code::unix:
      case Protocol::Code::onion3:
      case Protocol::Code::garlic64:
      case Protocol::Code::quic:
      case Protocol::Code::wss:
      case Protocol::Code::p2p_websocket_star:
      case Protocol::Code::p2p_stardust:
      case Protocol::Code::p2p_webrtc_direct:
      case Protocol::Code::p2p_circuit:
        return ConversionError::kNotImplemented;
      default:
        return ConversionError::kNoSuchProtocol;
    }
  }

  auto bytesToString(const Buffer &bytes) -> outcome::result<std::string> {
    std::string results;
    // Positioning for memory jump:
    int lastpos = 0;

    // set up variables
    const std::string hex = bytes.toHex();

    // Process Hex String
  NAX:
    gsl::span<const uint8_t, -1> pid_bytes{bytes.toVector()};
    int protocol_int = UVarint(pid_bytes.subspan(lastpos / 2)).toUInt64();
    Protocol const *protocol =
        ProtocolList::get(static_cast<Protocol::Code>(protocol_int));
    if (protocol != nullptr) {
      //////////Stage 2: Address
      if (protocol->name != "ipfs") {
        lastpos = lastpos
            + UVarint::calculateSize(pid_bytes.subspan(lastpos / 2)) * 2;
        std::string address;
        address = hex.substr(lastpos, protocol->size / 4);
        //////////Stage 3 Process it back to string
        lastpos = lastpos + (protocol->size / 4);

        //////////Address:
        results += "/";
        results += protocol->name;
        results += "/";

        // TODO(Akvinikym): 25.02.19 PRE-49: add more protocols
        if (protocol->name == "ip4") {
          results += boost::asio::ip::make_address_v4(std::stoul(address, nullptr, 16)).to_string();

        } else if (protocol->name == "tcp") {
          results += std::to_string(std::stoul(address, nullptr, 16));

        } else if (protocol->name == "udp") {
          results += std::to_string(std::stoul(address, nullptr, 16));

        } else {
          return ConversionError::kNotImplemented;
        }

      } else {
        lastpos = lastpos + 4;
        // fetch the size of the address based on the varint prefix
        auto prefixedvarint = hex.substr(lastpos, 2);
        auto prefixBytes = unhex(prefixedvarint);
        if (prefixBytes.hasError()) {
          return ConversionError::kInvalidAddress;
        }
        auto addrsize = UVarint(prefixBytes.getValueRef()).toUInt64();

        // get the ipfs address as hex values
        auto ipfsAddr = hex.substr(lastpos + 2, addrsize * 2);
        // convert the address from hex values to a binary array
        auto addrbufRes = unhex(ipfsAddr);
        if (addrbufRes.hasError()) {
          return ConversionError::kInvalidAddress;
        }
        auto &addrbuf = addrbufRes.getValueRef();
        auto encode_res = Base58Codec::encode(addrbuf);
        results += "/";
        results += protocol->name;
        results += "/" + encode_res;
        lastpos += addrsize * 2 + 2;
      }
      if (lastpos < bytes.size() * 2) {
        goto NAX;
      }
    } else {
      return ConversionError::kNoSuchProtocol;
    }
    results += "/";

    return results;
  }

}  // namespace libp2p::multi::converters
