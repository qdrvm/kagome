/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "converter_utils.hpp"

#include <ios>
#include <iosfwd>
#include <string>

#include <outcome/outcome.hpp>
#include "common/buffer.hpp"
#include "ip_v4_converter.hpp"
#include "ipfs_converter.hpp"
#include "tcp_converter.hpp"
#include "udp_converter.hpp"
#include "libp2p/multi/utils/protocol_list.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::multi::converters, ConversionError, e) {
  using libp2p::multi::converters::ConversionError;

  switch (e) {
    case ConversionError::kInvalidAddress:
      return "Invalid address";
    case ConversionError::kNoSuchProtocol:
      return "Protocol with given code does not exist";
    case ConversionError::kAddressDoesNotBeginWithSlash:
      return "An address should begin with a slash";
    case ConversionError::kNotImplemented:
      return "Conversion for this protocol is not implemented";
    default:
      return "Unknown error";
  }
}

namespace libp2p::multi::converters {

  using kagome::common::Buffer;

  std::string intToHex(uint64_t n) {
    std::stringstream result;
    result.width(2);
    result.fill('0');
    result << std::hex << std::uppercase << n;
    auto str = result.str();
    if (str.length() % 2 != 0) {
      str.push_back('\0');
      for (int i = str.length() - 2; i >= 0; --i) {
        str[i + 1] = str[i];
      }
      str[0] = '0';
    }
    return str;
  }

  outcome::result<Buffer> stringToBytes(std::string_view str) {
    if (str[0] != '/') {
      return ConversionError::kAddressDoesNotBeginWithSlash;
    }

    // Initializing variables to store our processed HEX in:
    std::string processed;  // HEX CONTAINER

    enum class WordType { Protocol, Address };
    // Now Setting up variables for calculating which is the first
    // and second word:
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
          processed += intToHex(static_cast<int>(protx->code));
          type = WordType::Address;  // Since the next word will be an address
        } else {
          return ConversionError::kNoSuchProtocol;
        }
      } else {
        std::string s_to_b;
        if (addressToBytes(*protx, str)) {
          return Error::kInvalidAddress;
        }

        int temp_size = processed.size();
        processed += s_to_b;
        processed[temp_size + s_to_b.size()] = 0;

        protx = nullptr;  // Since right now it doesn't need that
        // assignment anymore.
        type = WordType::Protocol;  // Since the next word will be an protocol
      }
      delim_pos = str.find("/");
      word = str.substr(0, delim_pos);
      str.remove_prefix(delim_pos + 1);
    }
    protx = nullptr;

    return Buffer{hexToBytes(processed)};
  }

  auto addressToBytes(const Protocol &protocol, const std::string &addr)
      -> outcome::result<std::string> {
    std::string astb__stringy;

    // TODO(Akvinikym) 25.02.19 PRE-49: add more protocols
    switch (protocol.code) {
      case Protocol::Code::ip4:
        return IPv4Converter::addressToBytes(protocol, addr);
      case Protocol::Code::tcp:
        return TcpConverter::addressToBytes(protocol, addr);
      case Protocol::Code::udp:
        return UdpConverter::addressToBytes(protocol, addr);
      case Protocol::Code::ipfs:
        return IpfsConverter::addressToBytes(protocol, addr);

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

  std::vector<uint8_t> hexToBytes(std::string_view hex_str) {
    std::vector<uint8_t> res;
    size_t incoming_size = hex_str.size();
    res.resize(incoming_size / 2);

    std::string code(2, ' ');
    code[2] = '\0';
    int pos = 0;
    for (int i = 0; i < incoming_size; i += 2) {
      code[0] = hex_str[i];
      code[1] = hex_str[i + 1];
      int64_t lu = std::stol(code);
      res[pos] = static_cast<unsigned char>(lu);
      pos++;
    }
    return res;
  }

}  // namespace libp2p::multi::converters
