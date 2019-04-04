/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_IPFS_CONVERTER_HPP
#define KAGOME_IPFS_CONVERTER_HPP

#include <boost/algorithm/string.hpp>

#include "common/buffer.hpp"
#include "common/hexutil.hpp"
#include "libp2p/multi/utils/uvarint.hpp"
#include "libp2p/multi/utils/base58_codec.hpp"

namespace libp2p::multi::converters {

  class IpfsConverter {
   public:
    static auto addressToBytes(const Protocol &protocol,
                               const std::string &addr)
        -> outcome::result<std::string> {
      auto res = Base58Codec::decode(addr);
      if (!res) {
        return res.error();
      }
      auto buf = res.value();
      // throw everything in a hex string so we can debug the results
      std::string addr_encoded;

      size_t ilen = 0;
      for (int i = 0; i < buf.size(); i++) {
        // get the char so we can see it in the debugger
        auto miu = intToHex(buf[i]);
        addr_encoded += miu;
      }
      ilen = addr_encoded.length();
      auto result = UVarint{ilen}.toHex() + addr_encoded;
      boost::algorithm::to_lower(result);
      return result;
    }
  };

}  // namespace libp2p::multi::converters

#endif  // KAGOME_IPFS_CONVERTER_HPP
