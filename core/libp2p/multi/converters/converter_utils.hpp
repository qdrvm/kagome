/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONVERTER_UTILS_HPP
#define KAGOME_CONVERTER_UTILS_HPP

#include <cmath>
#include <string>
#include <vector>

#include <outcome/outcome.hpp>
#include "common/buffer.hpp"
#include "libp2p/multi/utils/protocol_list.hpp"

namespace libp2p::multi::converters {

  enum class ConversionError {
    kAddressDoesNotBeginWithSlash = 1,
    kNoSuchProtocol,
    kInvalidAddress,
    kNotImplemented
  };

  std::string intToHex(uint64_t i);
  std::vector<uint8_t> hexToBytes(std::string_view hex_str);

  static uint64_t ipToInt(std::string_view r) {
    uint64_t final_result = 0;
    int ipat = 0;
    size_t c = 0;
    std::string::size_type prev = 0;
    auto pos = r.find(".");
    while (c < 4 && prev != std::string::npos) {
      ipat = std::stoi(std::string(r.substr(prev, pos)));
      prev = (pos != std::string::npos) ? pos+1 : pos;
      pos = r.find(".", prev);
      final_result += ipat * pow(2, (3 - c)*8);
      c++;
    }
    return final_result;
  }

  auto addressToBytes(const Protocol &protocol, const std::string &addr)
      -> outcome::result<std::string>;

  auto stringToBytes(std::string_view multiaddr_str)
      -> outcome::result<kagome::common::Buffer>;

}  // namespace libp2p::multi::converters

OUTCOME_HPP_DECLARE_ERROR(libp2p::multi::converters, ConversionError);

#endif  // KAGOME_CONVERTER_UTILS_HPP
