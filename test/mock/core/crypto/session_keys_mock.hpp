/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
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
    MOCK_METHOD(const std::shared_ptr<Sr25519Keypair> &,
                getBabeKeyPair,
                (),
                (override));

    MOCK_METHOD(const std::shared_ptr<Ed25519Keypair> &,
                getGranKeyPair,
                (),
                (override));

    MOCK_METHOD(const std::shared_ptr<Sr25519Keypair> &,
                getParaKeyPair,
                (),
                (override));

    MOCK_METHOD(std::shared_ptr<Sr25519Keypair>,
                getAudiKeyPair,
                (const std::vector<primitives::AuthorityDiscoveryId> &),
                (override));
  };
}  // namespace kagome::crypto

#endif  // KAGOME_CRYPTO_SESSIONKEYSMOCK
