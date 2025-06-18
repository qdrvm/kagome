/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "crypto/key_store/session_keys.hpp"

namespace kagome::application {
  class AppConfiguration;
}  // namespace kagome::application

namespace kagome::crypto {
  class SessionKeysMock : public SessionKeys {
   public:
    MOCK_METHOD(KeypairWithIndexOpt<Sr25519Keypair>,
                getBabeKeyPair,
                (const consensus::babe::Authorities &),
                (override));

    // MOCK_METHOD(KeypairWithIndexOpt<BandersnatchKeypair>,
    //             getSassafrasKeyPair,
    //             (const consensus::sassafras::Authorities &),
    //             (override));

    MOCK_METHOD(std::shared_ptr<Ed25519Keypair>,
                getGranKeyPair,
                (const consensus::grandpa::AuthoritySet &),
                (override));

    MOCK_METHOD(KeypairWithIndexOpt<Sr25519Keypair>,
                getParaKeyPair,
                (const std::vector<Sr25519PublicKey> &),
                (override));

    MOCK_METHOD(std::vector<Sr25519Keypair>,
                getAudiKeyPairs,
                (const std::vector<primitives::AuthorityDiscoveryId> &),
                (override));

    MOCK_METHOD(std::vector<Sr25519Keypair>, getAudiKeyPairs, (), (override));

    MOCK_METHOD(KeypairWithIndexOpt<EcdsaKeypair>,
                getBeefKeyPair,
                (const std::vector<EcdsaPublicKey> &),
                (override));
  };
}  // namespace kagome::crypto
