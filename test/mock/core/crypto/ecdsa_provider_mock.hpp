/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CORE_CRYPTO_ECDSA_PROVIDER_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_CRYPTO_ECDSA_PROVIDER_MOCK_HPP

#include "crypto/ecdsa_provider.hpp"

#include <gmock/gmock.h>

namespace kagome::crypto {

  class EcdsaProviderMock : public EcdsaProvider {
   public:
    EcdsaKeypair;

    MOCK_METHOD(outcome::result<EcdsaKeypair>, generate, (), (const, override));

    MOCK_METHOD(outcome::result<EcdsaPublicKey>,
                derive,
                (const EcdsaSeed &),
                (const, override));

    MOCK_METHOD(outcome::result<EcdsaSignature>,
                sign,
                (gsl::span<const uint8_t>, const EcdsaPrivateKey &),
                (const, override));

    MOCK_METHOD(outcome::result<bool>,
                verify,
                (gsl::span<const uint8_t>,
                 const EcdsaSignature &,
                 const EcdsaPublicKey &),
                (const, override));
  };

}  // namespace kagome::crypto

#endif  // KAGOME_TEST_MOCK_CORE_CRYPTO_ECDSA_PROVIDER_MOCK_HPP
