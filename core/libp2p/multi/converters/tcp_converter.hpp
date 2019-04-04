/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TCPCONVERTER_HPP
#define KAGOME_TCPCONVERTER_HPP

#include <outcome/outcome.hpp>
#include "common/buffer.hpp"
#include "libp2p/multi/utils/protocol_list.hpp"

namespace libp2p::multi::converters {

  class TcpConverter {
   public:
    static auto addressToBytes(const Protocol &protocol,
                               const std::string &addr)
        -> outcome::result<std::string> {
      auto n = std::stoi(addr);
      if (n < 65536 && n > 0) {
        std::string himm_woot = intToHex((uint64_t)n);
        himm_woot.resize(4, '\0');
        if (himm_woot[2] == '\0') {
          char swap0 = '0';
          char swap1 = '0';
          char swap2 = himm_woot[0];
          char swap3 = himm_woot[1];
          himm_woot[0] = swap0;
          himm_woot[1] = swap1;
          himm_woot[2] = swap2;
          himm_woot[3] = swap3;
        } else if (himm_woot[3] == '\0') {
          char swap0 = '0';
          char swap1 = himm_woot[0];
          char swap2 = himm_woot[1];
          char swap3 = himm_woot[2];
          himm_woot[0] = swap0;
          himm_woot[1] = swap1;
          himm_woot[2] = swap2;
          himm_woot[3] = swap3;
        }

        return himm_woot;
      }
      return ConversionError::kInvalidAddress;
    }
  };

}  // namespace libp2p::multi::converters

#endif  // KAGOME_TCPCONVERTER_HPP
