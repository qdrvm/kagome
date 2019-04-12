/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/multi/converters/tcp_converter.hpp"

#include <string>

#include <outcome/outcome.hpp>
#include "common/hexutil.hpp"
#include "libp2p/multi/converters/conversion_error.hpp"

namespace libp2p::multi::converters {

  auto TcpConverter::addressToHex(std::string_view addr)
      -> outcome::result<std::string> {
    int64_t n = 0;
    if(!std::all_of(addr.begin(), addr.end(), [](char c) { return std::isdigit(c) == 1; })) {
      return ConversionError::INVALID_ADDRESS;
    }
    try {
      n = std::stoi(std::string(addr));
    } catch(std::exception& e) {
      return ConversionError::INVALID_ADDRESS;
    }
    if (n == 0) {
      return "0000";
    }
    if (n < 65536 && n > 0) {
      return kagome::common::int_to_hex(n, 4);
    }
    return ConversionError::INVALID_ADDRESS;
  }
}  // namespace libp2p::multi::converters
