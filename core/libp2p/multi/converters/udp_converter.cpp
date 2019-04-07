/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/multi/converters/udp_converter.hpp"

#include <outcome/outcome.hpp>
#include "libp2p/multi/converters/conversion_error.hpp"
#include "libp2p/multi/utils/multi_hex_utils.hpp"
#include "libp2p/multi/utils/protocol_list.hpp"

namespace libp2p::multi::converters {

  auto UdpConverter::addressToBytes(std::string_view addr)
      -> outcome::result<std::string> {
    auto n = std::stoi(std::string(addr));
    if (n == 0) {
      return "0000";
    }
    if (n < 65536 && n > 0) {
      std::string himm_woot2 = intToHex(n);
      himm_woot2.resize(4, '\0');
      if (himm_woot2[2] == '\0') {  // Manual Switch2be
        char swap0 = '0';
        char swap1 = '0';
        char swap2 = himm_woot2[0];
        char swap3 = himm_woot2[1];
        himm_woot2[0] = swap0;
        himm_woot2[1] = swap1;
        himm_woot2[2] = swap2;
        himm_woot2[3] = swap3;
      } else if (himm_woot2[3] == '\0') {  // Manual switch
        char swap0 = '0';
        char swap1 = himm_woot2[0];
        char swap2 = himm_woot2[1];
        char swap3 = himm_woot2[2];
        himm_woot2[0] = swap0;
        himm_woot2[1] = swap1;
        himm_woot2[2] = swap2;
        himm_woot2[3] = swap3;
      }
      return himm_woot2;
    }
    return ConversionError::kInvalidAddress;
  }
}  // namespace libp2p::multi::converters
