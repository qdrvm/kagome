/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_IPV4CONVERTER_HPP
#define KAGOME_IPV4CONVERTER_HPP

#include <outcome/outcome.hpp>
#include "common/buffer.hpp"
#include "libp2p/multi/utils/ip_v4_validator.hpp"
#include "libp2p/multi/utils/protocol_list.hpp"

namespace libp2p::multi::converters {

  /**
   * Converts an ip4 part of a multiaddress (an IP address)
   * to bytes representation as a hex string
   */
  class IPv4Converter {
   public:
    static auto addressToBytes(std::string_view addr)
        -> outcome::result<std::string>;
  };

}  // namespace libp2p::multi::converters

#endif  // KAGOME_IPV4CONVERTER_HPP
