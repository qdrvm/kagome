/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

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
                (BufferView, const EcdsaPrivateKey &),
                (const, override));

    MOCK_METHOD(outcome::result<bool>,
                verify,
                (BufferView, const EcdsaSignature &, const EcdsaPublicKey &),
                (const, override));
  };

}  // namespace kagome::crypto
