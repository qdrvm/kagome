/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_STORE_MOCK_HPP
#define KAGOME_CRYPTO_STORE_MOCK_HPP

#include "crypto/crypto_store.hpp"

#include <gmock/gmock.h>

namespace kagome::crypto {
  class CryptoStoreMock : public CryptoStore {
   public:
    ~CryptoStoreMock() override = default;

    MOCK_METHOD2(generateEd25519Keypair,
                 outcome::result<ED25519Keypair>(KeyTypeId, std::string_view));
    MOCK_METHOD2(generateSr25519Keypair,
                 outcome::result<SR25519Keypair>(KeyTypeId, std::string_view));
    MOCK_METHOD2(generateEd25519Keypair,
                 ED25519Keypair(KeyTypeId, const ED25519Seed &));
    MOCK_METHOD2(generateSr25519Keypair,
                 SR25519Keypair(KeyTypeId, const SR25519Seed &));
    MOCK_METHOD1(generateEd25519Keypair,
                 outcome::result<ED25519Keypair>(KeyTypeId));
    MOCK_METHOD1(generateSr25519Keypair,
                 outcome::result<SR25519Keypair>(KeyTypeId));
    MOCK_CONST_METHOD2(
        findEd25519Keypair,
        outcome::result<ED25519Keypair>(KeyTypeId, const ED25519PublicKey &));
    MOCK_CONST_METHOD2(
        findSr25519Keypair,
        outcome::result<SR25519Keypair>(KeyTypeId, const SR25519PublicKey &));
    MOCK_CONST_METHOD1(getEd25519PublicKeys, ED25519Keys(KeyTypeId));
    MOCK_CONST_METHOD1(getSr25519PublicKeys, SR25519Keys(KeyTypeId));
  };
}  // namespace kagome::crypto

#endif  // KAGOME_CRYPTO_STORE_MOCK_HPP
