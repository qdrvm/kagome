/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CRYPTO_ED29919_PROVIDER_HPP
#define KAGOME_CORE_CRYPTO_ED29919_PROVIDER_HPP

#include <gsl/span.h>
#include <outcome/outcome.hpp>
#include "crypto/ed25519_types.hpp"

namespace kagome::crypto {
  class ED25519Provider {
   public:
    virtual ~ED25519Provider() = default;

    virtual ED25519KeyPair generateKeyPair() = 0;

    virtual outcome::result<ED25519Signature> sign(
        gsl::span<uint8_t> message, const ED25519KeyPair &key_pair) = 0;

    virtual bool verify(gsl::span<uint8_t> signature,
                        gsl::span<uint8_t> message,
                        const ED25519PublicKey &public_key) = 0;
  };
}  // namespace kagome::crypto

#endif  // KAGOME_CORE_CRYPTO_ED29919_PROVIDER_HPP
