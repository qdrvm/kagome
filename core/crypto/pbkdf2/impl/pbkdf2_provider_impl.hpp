/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_PBKDF2_PROVIDER_IMPL_HPP
#define KAGOME_CRYPTO_PBKDF2_PROVIDER_IMPL_HPP

#include "crypto/pbkdf2/pbkdf2_provider.hpp"

namespace kagome::crypto {

  class Pbkdf2ProviderImpl : public Pbkdf2Provider {
   public:
    ~Pbkdf2ProviderImpl() override = default;

    outcome::result<common::Buffer> deriveKey(gsl::span<const uint8_t> data,
                                              gsl::span<const uint8_t> salt,
                                              size_t iterations,
                                              size_t key_length) const override;
  };

}  // namespace kagome::crypto

#endif  // KAGOME_CRYPTO_PBKDF2_PROVIDER_IMPL_HPP
