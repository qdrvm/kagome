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
                 outcome::result<Ed25519Keypair>(KeyTypeId, std::string_view));
    MOCK_METHOD2(generateSr25519Keypair,
                 outcome::result<Sr25519Keypair>(KeyTypeId, std::string_view));
    MOCK_METHOD2(generateEd25519Keypair,
                 Ed25519Keypair(KeyTypeId, const Ed25519Seed &));
    MOCK_METHOD2(generateSr25519Keypair,
                 Sr25519Keypair(KeyTypeId, const Sr25519Seed &));
    MOCK_METHOD1(generateEd25519KeypairOnDisk,
                 outcome::result<Ed25519Keypair>(KeyTypeId));
    MOCK_METHOD1(generateSr25519KeypairOnDisk,
                 outcome::result<Sr25519Keypair>(KeyTypeId));
    MOCK_CONST_METHOD2(
        findEd25519Keypair,
        outcome::result<Ed25519Keypair>(KeyTypeId, const Ed25519PublicKey &));
    MOCK_CONST_METHOD2(
        findSr25519Keypair,
        outcome::result<Sr25519Keypair>(KeyTypeId, const Sr25519PublicKey &));
    MOCK_CONST_METHOD1(getEd25519PublicKeys,
                       outcome::result<Ed25519Keys>(KeyTypeId));
    MOCK_CONST_METHOD1(getSr25519PublicKeys,
                       outcome::result<Sr25519Keys>(KeyTypeId));

    MOCK_CONST_METHOD0(getLibp2pKeypair,
                       std::optional<libp2p::crypto::KeyPair>());
    MOCK_CONST_METHOD1(loadLibp2pKeypair,
                       outcome::result<libp2p::crypto::KeyPair>(const Path &));
  };
}  // namespace kagome::crypto

#endif  // KAGOME_CRYPTO_STORE_MOCK_HPP
