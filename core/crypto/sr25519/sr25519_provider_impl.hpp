/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CRYPTO_SR25519_SR25519_PROVIDER_IMPL_HPP
#define KAGOME_CORE_CRYPTO_SR25519_SR25519_PROVIDER_IMPL_HPP

#include "crypto/sr25519_provider.hpp"

namespace kagome::crypto {

  class SR25519ProviderImpl : public SR25519Provider {
   public:
    ~SR25519ProviderImpl() override = default;

  };

}  // namespace kagome::crypto

#endif  // KAGOME_CORE_CRYPTO_SR25519_SR25519_PROVIDER_IMPL_HPP
