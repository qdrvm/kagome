/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_IPV4CONVERTER_HPP
#define KAGOME_IPV4CONVERTER_HPP

#include <outcome/outcome.hpp>
#include "common/buffer.hpp"
#include "libp2p/multi/utils/protocol_list.hpp"
#include "libp2p/multi/utils/ip_v4_validator.hpp"

namespace libp2p::multi::converters {

  class IPv4Converter {
  public:
    static auto addressToBytes(const Protocol &protocol, const std::string &addr)
       -> outcome::result <std::string> {
      if (IPv4Validator::isValidIp(addr)) {
        uint64_t iip = ipToInt(addr);
        return intToHex(iip);
      }
      return ConversionError::kInvalidAddress;
    }
  };

}

#endif //KAGOME_IPV4CONVERTER_HPP
