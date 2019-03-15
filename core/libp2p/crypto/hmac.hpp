/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_HMAC_HPP
#define KAGOME_HMAC_HPP

#include "common/buffer.hpp"
#include "libp2p/crypto/crypto_common.hpp"

namespace libp2p::crypto {
  /**
   * Keyed-Hash Message Authentication Code
   */
  class Hmac {
   public:
    /**
     * Create an instance of Hmac with such hash and key
     * @param hash to be used
     * @param secret to be used
     */
    Hmac(common::HashType hash, const kagome::common::Buffer &secret);

    /**
     * Hash the data
     * @param data to be hashed
     * @return hashed bytes
     */
    kagome::common::Buffer digest(const kagome::common::Buffer &data);
  };
}  // namespace libp2p::crypto

#endif  // KAGOME_HMAC_HPP
