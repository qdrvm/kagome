/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_LIBP2P_CRYPTO_HMAC_HMAC_PROVIDER_HPP
#define KAGOME_CORE_LIBP2P_CRYPTO_HMAC_HMAC_PROVIDER_HPP

#include <outcome/outcome.hpp>

#include "common/buffer.hpp"
#include "libp2p/crypto/common.hpp"

namespace libp2p::crypto::hmac {
  class HmacProvider {
    using HashType = common::HashType;
    using Buffer = kagome::common::Buffer;

   public:
    /**
     * @brief calculates digests
     * @param hash_type hash type
     * @param key secret key
     * @param message source message
     * @return message digest
     */
    outcome::result<Buffer> calculateDigest(HashType hash_type,
                                            const Buffer &key,
                                            const Buffer &message) const;
  };
}  // namespace libp2p::crypto::hmac

#endif  // KAGOME_CORE_LIBP2P_CRYPTO_HMAC_HMAC_PROVIDER_HPP
