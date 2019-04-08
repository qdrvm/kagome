/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/multi/converters/ipfs_converter.hpp"

#include <string>

#include <outcome/outcome.hpp>
#include "libp2p/multi/converters/conversion_error.hpp"
#include "common/hexutil.hpp"

namespace libp2p::multi::converters {

  auto IpfsConverter::addressToBytes(std::string_view addr)
      -> outcome::result<std::string> {
    auto res = Base58Codec::decode(addr);
    if (!res) {
      return res.error();
    }
    auto buf = res.value();
    // throw everything in a hex string so we can debug the results
    std::string addr_encoded;

    size_t ilen = 0;
    for (uint8_t const &elem : buf) {
      // get the char so we can see it in the debugger
      auto miu = kagome::common::int_to_hex(elem);
      addr_encoded += miu;
    }
    ilen = addr_encoded.length() / 2;
    auto result = UVarint{ilen}.toHex() + addr_encoded;
    return result;
  }

}  // namespace libp2p::multi::converters
