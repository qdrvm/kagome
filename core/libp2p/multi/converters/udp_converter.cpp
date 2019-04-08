/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/multi/converters/udp_converter.hpp"

#include <outcome/outcome.hpp>
#include "libp2p/multi/converters/conversion_error.hpp"
#include "libp2p/multi/utils/protocol_list.hpp"
#include "common/hexutil.hpp"

namespace libp2p::multi::converters {

  auto UdpConverter::addressToBytes(std::string_view addr)
      -> outcome::result<std::string> {
    auto n = std::stoi(std::string(addr));
    if (n == 0) {
      return "0000";
    }
    if (n < 65536 && n > 0) {
      std::stringstream ss;
      ss.width(4);
      ss.fill('0');
      ss << std::hex << std::uppercase << n;

      return ss.str();
    }
    return ConversionError::kInvalidAddress;
  }
}  // namespace libp2p::multi::converters
