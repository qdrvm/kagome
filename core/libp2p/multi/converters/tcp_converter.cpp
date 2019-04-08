/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/multi/converters/tcp_converter.hpp"

#include <string>

#include <outcome/outcome.hpp>
#include "libp2p/multi/converters/conversion_error.hpp"
#include "common/hexutil.hpp"

namespace libp2p::multi::converters {

  auto TcpConverter::addressToBytes(std::string_view addr)
      -> outcome::result<std::string> {
    int64_t n = std::stoi(std::string(addr));
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
