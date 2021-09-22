/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_CRYPTO_SR25510_PROVIDER_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_CRYPTO_SR25510_PROVIDER_MOCK_HPP

#include <gmock/gmock.h>
#include "crypto/sr25519_provider.hpp"

namespace kagome::crypto {
  struct Sr25519ProviderMock : public Sr25519Provider {
    MOCK_CONST_METHOD0(generateKeypair, Sr25519Keypair());
    MOCK_CONST_METHOD1(generateKeypair, Sr25519Keypair(const Sr25519Seed &seed));

    MOCK_CONST_METHOD2(sign,
                       outcome::result<Sr25519Signature>(const Sr25519Keypair &,
                                                         gsl::span<const uint8_t>));

    MOCK_CONST_METHOD3(verify,
                       outcome::result<bool>(const Sr25519Signature &,
                                             gsl::span<const uint8_t>,
                                             const Sr25519PublicKey &));
     MOCK_CONST_METHOD3(verify_deprecated,
                       outcome::result<bool>(const Sr25519Signature &,
                                             gsl::span<const uint8_t>,
                                             const Sr25519PublicKey &));
  };
}  // namespace kagome::crypto

#endif  // KAGOME_TEST_MOCK_CORE_CRYPTO_SR25510_PROVIDER_MOCK_HPP
