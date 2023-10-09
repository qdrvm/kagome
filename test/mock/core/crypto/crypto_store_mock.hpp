/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "crypto/crypto_store.hpp"

#include <gmock/gmock.h>

namespace kagome::crypto {
  class CryptoStoreMock : public CryptoStore {
   public:
    ~CryptoStoreMock() override = default;

    MOCK_METHOD(outcome::result<EcdsaKeypair>,
                generateEcdsaKeypair,
                (KeyType, std::string_view),
                (override));

    MOCK_METHOD(outcome::result<EcdsaKeypair>,
                generateEcdsaKeypair,
                (KeyType, const EcdsaSeed &),
                (override));

    MOCK_METHOD(outcome::result<EcdsaKeypair>,
                generateEcdsaKeypairOnDisk,
                (KeyType),
                (override));

    MOCK_METHOD(outcome::result<EcdsaKeypair>,
                findEcdsaKeypair,
                (KeyType, const EcdsaPublicKey &),
                (const, override));

    MOCK_METHOD(outcome::result<EcdsaKeys>,
                getEcdsaPublicKeys,
                (KeyType),
                (const, override));

    MOCK_METHOD(outcome::result<Ed25519Keypair>,
                generateEd25519Keypair,
                (KeyType, std::string_view),
                (override));

    MOCK_METHOD(outcome::result<Sr25519Keypair>,
                generateSr25519Keypair,
                (KeyType, std::string_view),
                (override));

    MOCK_METHOD(outcome::result<Ed25519Keypair>,
                generateEd25519Keypair,
                (KeyType, const Ed25519Seed &),
                (override));

    MOCK_METHOD(outcome::result<Sr25519Keypair>,
                generateSr25519Keypair,
                (KeyType, const Sr25519Seed &),
                (override));

    MOCK_METHOD(outcome::result<Ed25519Keypair>,
                generateEd25519KeypairOnDisk,
                (KeyType),
                (override));

    MOCK_METHOD(outcome::result<Sr25519Keypair>,
                generateSr25519KeypairOnDisk,
                (KeyType),
                (override));

    MOCK_METHOD(outcome::result<Ed25519Keypair>,
                findEd25519Keypair,
                (KeyType, const Ed25519PublicKey &),
                (const, override));

    MOCK_METHOD(outcome::result<Sr25519Keypair>,
                findSr25519Keypair,
                (KeyType, const Sr25519PublicKey &),
                (const, override));

    MOCK_METHOD(outcome::result<Ed25519Keys>,
                getEd25519PublicKeys,
                (KeyType),
                (const, override));

    MOCK_METHOD(outcome::result<Sr25519Keys>,
                getSr25519PublicKeys,
                (KeyType),
                (const, override));

    MOCK_METHOD(outcome::result<libp2p::crypto::KeyPair>,
                loadLibp2pKeypair,
                (const Path &),
                (const, override));
  };
}  // namespace kagome::crypto
