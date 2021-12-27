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

    MOCK_METHOD(outcome::result<EcdsaKeypair>,
                generateEcdsaKeypair,
                (KeyTypeId, std::string_view),
                (override));

    MOCK_METHOD(EcdsaKeypair,
                generateEcdsaKeypair,
                (KeyTypeId, const EcdsaSeed &),
                (override));

    MOCK_METHOD(outcome::result<EcdsaKeypair>,
                generateEcdsaKeypairOnDisk,
                (KeyTypeId),
                (override));

    MOCK_METHOD(outcome::result<EcdsaKeypair>,
                findEcdsaKeypair,
                (KeyTypeId, const EcdsaPublicKey &),
                (const, override));

    MOCK_METHOD(outcome::result<EcdsaKeys>,
                getEcdsaPublicKeys,
                (KeyTypeId),
                (const, override));

    MOCK_METHOD(outcome::result<Ed25519Keypair>,
                generateEd25519Keypair,
                (KeyTypeId, std::string_view),
                (override));

    MOCK_METHOD(outcome::result<Sr25519Keypair>,
                generateSr25519Keypair,
                (KeyTypeId, std::string_view),
                (override));

    MOCK_METHOD(Ed25519Keypair,
                generateEd25519Keypair,
                (KeyTypeId, const Ed25519Seed &),
                (override));

    MOCK_METHOD(Sr25519Keypair,
                generateSr25519Keypair,
                (KeyTypeId, const Sr25519Seed &),
                (override));

    MOCK_METHOD(outcome::result<Ed25519Keypair>,
                generateEd25519KeypairOnDisk,
                (KeyTypeId),
                (override));

    MOCK_METHOD(outcome::result<Sr25519Keypair>,
                generateSr25519KeypairOnDisk,
                (KeyTypeId),
                (override));

    MOCK_METHOD(outcome::result<Ed25519Keypair>,
                findEd25519Keypair,
                (KeyTypeId, const Ed25519PublicKey &),
                (const, override));

    MOCK_METHOD(outcome::result<Sr25519Keypair>,
                findSr25519Keypair,
                (KeyTypeId, const Sr25519PublicKey &),
                (const, override));

    MOCK_METHOD(outcome::result<Ed25519Keys>,
                getEd25519PublicKeys,
                (KeyTypeId),
                (const, override));

    MOCK_METHOD(outcome::result<Sr25519Keys>,
                getSr25519PublicKeys,
                (KeyTypeId),
                (const, override));

    MOCK_METHOD(std::optional<libp2p::crypto::KeyPair>,
                getLibp2pKeypair,
                (),
                (const, override));

    MOCK_METHOD(outcome::result<libp2p::crypto::KeyPair>,
                loadLibp2pKeypair,
                (const Path &),
                (const, override));
  };
}  // namespace kagome::crypto

#endif  // KAGOME_CRYPTO_STORE_MOCK_HPP
