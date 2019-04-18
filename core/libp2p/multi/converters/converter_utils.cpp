/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/multi/converters/converter_utils.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <outcome/outcome.hpp>
#include "common/buffer.hpp"
#include "common/hexutil.hpp"
#include "libp2p/multi/converters/conversion_error.hpp"
#include "libp2p/multi/converters/ip_v4_converter.hpp"
#include "libp2p/multi/converters/ipfs_converter.hpp"
#include "libp2p/multi/converters/tcp_converter.hpp"
#include "libp2p/multi/converters/udp_converter.hpp"
#include "libp2p/multi/multiaddress_protocol_list.hpp"
#include "libp2p/multi/multibase_codec/multibase_codec_impl.hpp"
#include "libp2p/multi/uvarint.hpp"

using kagome::common::unhex;

namespace libp2p::multi::converters {

  using kagome::common::Buffer;

  outcome::result<Buffer> multiaddrToBytes(std::string_view str) {
    if (str[0] != '/') {
      return ConversionError::ADDRESS_DOES_NOT_BEGIN_WITH_SLASH;
    }

    str.remove_prefix(1);
    if (str.empty()) {
      return ConversionError ::INVALID_ADDRESS;
    }

    if (str.back() == '/') {
      // for split not to recognize an empty token in the end
      str.remove_suffix(1);
    }

    std::string processed;

    enum class WordType { Protocol, Address };
    WordType type = WordType::Protocol;

    Protocol const *protx = nullptr;

    std::list<std::string> tokens;
    boost::algorithm::split(tokens, str, boost::algorithm::is_any_of("/"));

    for (auto &word : tokens) {
      if (type == WordType::Protocol) {
        protx = ProtocolList::get(word);
        if (protx != nullptr) {
          processed += UVarint(static_cast<uint64_t>(protx->code)).toHex();
          type = WordType::Address;  // Since the next word will be an address
        } else {
          return ConversionError::NO_SUCH_PROTOCOL;
        }
      } else {
        OUTCOME_TRY(val, addressToHex(*protx, word));
        processed += val;
        protx = nullptr;  // Since right now it doesn't need that
        // assignment anymore.
        type = WordType::Protocol;  // Since the next word will be an protocol
      }
    }

    OUTCOME_TRY(bytes, unhex(processed));
    return Buffer{std::move(bytes)};
  }

  auto addressToHex(const Protocol &protocol, std::string_view addr)
      -> outcome::result<std::string> {
    std::string astb__stringy;

    // TODO(Akvinikym) 25.02.19 PRE-49: add more protocols
    switch (protocol.code) {
      case Protocol::Code::ip4:
        return IPv4Converter::addressToHex(addr);
      case Protocol::Code::tcp:
        return TcpConverter::addressToHex(addr);
      case Protocol::Code::udp:
        return UdpConverter::addressToHex(addr);
      case Protocol::Code::ipfs:
        return IpfsConverter::addressToHex(addr);

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
        return ConversionError::NOT_IMPLEMENTED;
      default:
        return ConversionError::NO_SUCH_PROTOCOL;
    }
  }

  auto bytesToMultiaddrString(const Buffer &bytes)
      -> outcome::result<std::string> {
    std::string results;

    size_t lastpos = 0;

    // set up variables
    const std::string hex = bytes.toHex();

    // Process Hex String
    while (lastpos < bytes.size() * 2) {
      gsl::span<const uint8_t, -1> pid_bytes{bytes.toVector()};
      int protocol_int = UVarint(pid_bytes.subspan(lastpos / 2)).toUInt64();
      Protocol const *protocol =
          ProtocolList::get(static_cast<Protocol::Code>(protocol_int));
      if (protocol == nullptr) {
        return ConversionError::NO_SUCH_PROTOCOL;
      }

      if (protocol->name != "ipfs") {
        lastpos = lastpos
            + UVarint::calculateSize(pid_bytes.subspan(lastpos / 2)) * 2;
        std::string address;
        address = hex.substr(lastpos, protocol->size / 4);

        lastpos = lastpos + (protocol->size / 4);

        results += "/";
        results += protocol->name;
        results += "/";

        // TODO(Akvinikym): 25.02.19 PRE-49: add more protocols
        if (protocol->name == "ip4") {
          results +=
              boost::asio::ip::make_address_v4(std::stoul(address, nullptr, 16))
                  .to_string();

        } else if (protocol->name == "tcp") {
          results += std::to_string(std::stoul(address, nullptr, 16));

        } else if (protocol->name == "udp") {
          results += std::to_string(std::stoul(address, nullptr, 16));

        } else {
          return ConversionError::NOT_IMPLEMENTED;
        }

      } else {
        lastpos = lastpos + 4;
        // fetch the size of the address based on the varint prefix
        auto prefixedvarint = hex.substr(lastpos, 2);
        OUTCOME_TRY(prefixBytes, unhex(prefixedvarint));

        auto addrsize = UVarint(prefixBytes).toUInt64();

        // get the ipfs address as hex values
        auto ipfsAddr = hex.substr(lastpos + 2, addrsize * 2);

        // convert the address from hex values to a binary array
        OUTCOME_TRY(addrbuf, Buffer::fromHex(ipfsAddr));
        auto encode_res = MultibaseCodecImpl{}.encode(
            addrbuf, MultibaseCodecImpl::Encoding::BASE58);
        encode_res.erase(0, 1);  // because multibase contains a char that
                                 // denotes which base is used
        results += "/";
        results += protocol->name;
        results += "/" + encode_res;
        lastpos += addrsize * 2 + 2;
      }
    }
    results += "/";

    return results;
  }

}  // namespace libp2p::multi::converters
