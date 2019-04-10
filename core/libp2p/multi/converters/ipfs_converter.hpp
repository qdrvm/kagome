/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_IPFS_CONVERTER_HPP
#define KAGOME_IPFS_CONVERTER_HPP

#include <outcome/outcome.hpp>
#include "common/buffer.hpp"
#include "common/hexutil.hpp"
#include "libp2p/multi/utils/uvarint.hpp"

namespace libp2p::multi::converters {

  /**
   * Converts an ipfs part of a multiaddress (encoded in base58)
   * to bytes representation as a hex string
   */
  class IpfsConverter {
   public:
    static auto addressToHex(std::string_view addr)
        -> outcome::result<std::string>;
  };

}  // namespace libp2p::multi::converters

#endif  // KAGOME_IPFS_CONVERTER_HPP
