/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_CRYPTO_ED25519_PROVIDER_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_CRYPTO_ED25519_PROVIDER_MOCK_HPP

#include "crypto/ed25519_provider.hpp"

#include <gmock/gmock.h>

namespace kagome::crypto {

  class ED25519ProviderMock : public ED25519Provider {
   public:
    MOCK_CONST_METHOD0(generateKeypair, outcome::result<ED25519Keypair>());
    MOCK_CONST_METHOD1(generateKeypair,
                       ED25519Keypair(const ED25519Seed &));
    MOCK_CONST_METHOD2(sign,
                       outcome::result<ED25519Signature>(const ED25519Keypair &,
                                                         gsl::span<uint8_t>));
    MOCK_CONST_METHOD3(
        verify,
        outcome::result<bool>(const ED25519Signature &signature,
                              gsl::span<uint8_t> message,
                              const ED25519PublicKey &public_key));
  };

}  // namespace kagome::crypto

#endif  // KAGOME_TEST_MOCK_CORE_CRYPTO_ED25519_PROVIDER_MOCK_HPP
