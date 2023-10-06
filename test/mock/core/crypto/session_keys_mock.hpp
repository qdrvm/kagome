/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_SESSIONKEYSMOCK
#define KAGOME_CRYPTO_SESSIONKEYSMOCK

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "crypto/crypto_store/session_keys.hpp"

namespace kagome::application {
  class AppConfiguration;
}  // namespace kagome::application

namespace kagome::crypto {
  class SessionKeysMock : public SessionKeys {
   public:
    MOCK_METHOD(KeypairWithIndexOpt<Sr25519Keypair>,
                getBabeKeyPair,
                (const primitives::AuthorityList &),
                (override));

    MOCK_METHOD(std::shared_ptr<Ed25519Keypair>,
                getGranKeyPair,
                (const primitives::AuthoritySet &),
                (override));

    MOCK_METHOD(KeypairWithIndexOpt<Sr25519Keypair>,
                getParaKeyPair,
                (const std::vector<Sr25519PublicKey> &),
                (override));

    MOCK_METHOD(std::shared_ptr<Sr25519Keypair>,
                getAudiKeyPair,
                (const std::vector<primitives::AuthorityDiscoveryId> &),
                (override));

    MOCK_METHOD(KeypairWithIndexOpt<EcdsaKeypair>,
                getBeefKeyPair,
                (const std::vector<EcdsaPublicKey> &),
                (override));
  };
}  // namespace kagome::crypto

#endif  // KAGOME_CRYPTO_SESSIONKEYSMOCK
