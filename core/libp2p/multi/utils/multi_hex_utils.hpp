/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_LIBP2P_MULTI_UTILS_MULTI_HEX_UTILS_HPP_
#define KAGOME_CORE_LIBP2P_MULTI_UTILS_MULTI_HEX_UTILS_HPP_

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace libp2p::multi {

  std::string intToHex(uint64_t i);
  uint64_t hexToInt(std::string_view hex);

  std::vector<uint8_t> hexToBytes(std::string_view hex_str);

  std::string intToIp(uint64_t anInt);
  uint64_t ipToInt(std::string_view r);

}

#endif  // KAGOME_CORE_LIBP2P_MULTI_UTILS_MULTI_HEX_UTILS_HPP_
