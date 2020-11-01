/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_CRYPTO_ED25519_PROVIDER_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_CRYPTO_ED25519_PROVIDER_MOCK_HPP

#include "crypto/ed25519_provider.hpp"

#include <gmock/gmock.h>

namespace kagome::crypto {

  class Ed25519ProviderMock : public Ed25519Provider {
   public:
    Ed25519Keypair;
    Ed25519Keypair;
    MOCK_CONST_METHOD2(sign,
                       outcome::result<Ed25519Signature>(const Ed25519Keypair &,
                                                         gsl::span<uint8_t>));
    MOCK_CONST_METHOD3(
        verify,
        outcome::result<bool>(const Ed25519Signature &signature,
                              gsl::span<uint8_t> message,
                              const Ed25519PublicKey &public_key));
  };

}  // namespace kagome::crypto

#endif  // KAGOME_TEST_MOCK_CORE_CRYPTO_ED25519_PROVIDER_MOCK_HPP
