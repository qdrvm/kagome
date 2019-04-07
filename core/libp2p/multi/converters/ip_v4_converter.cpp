/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/multi/converters/ip_v4_converter.hpp"

#include <outcome/outcome.hpp>
#include <string>
#include "libp2p/multi/converters/conversion_error.hpp"
#include "libp2p/multi/utils/multi_hex_utils.hpp"
#include "libp2p/multi/utils/protocol_list.hpp"

namespace libp2p::multi::converters {

  auto IPv4Converter::addressToBytes(std::string_view addr)
      -> outcome::result<std::string> {
    if (IPv4Validator::isValidIp(addr)) {
      uint64_t iip = ipToInt(addr);
      auto hex = intToHex(iip);
      hex = std::string(8 - hex.length(), '0') + hex;
      return hex;
    }
    return ConversionError::kInvalidAddress;
  }

}  // namespace libp2p::multi::converters
