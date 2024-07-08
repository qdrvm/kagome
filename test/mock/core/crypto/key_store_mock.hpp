/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "crypto/ed25519_provider.hpp"
#include "crypto/key_store.hpp"
#include "mock/core/application/app_state_manager_mock.hpp"
#include "mock/core/crypto/ed25519_provider_mock.hpp"

#include <gmock/gmock.h>
#include <memory>

namespace kagome::crypto {

  template <Suite T>
  class KeySuiteStoreMock : public KeySuiteStore<T> {
   public:
    using Keypair = typename T::Keypair;
    using PrivateKey = typename T::PrivateKey;
    using PublicKey = typename T::PublicKey;
    using Seed = typename T::Seed;

    MOCK_METHOD(outcome::result<Keypair>,
                generateKeypair,
                (KeyType, std::string_view),
                (override));

    MOCK_METHOD(outcome::result<Keypair>,
                generateKeypair,
                (KeyType, const Seed &seed),
                (override));

    MOCK_METHOD(outcome::result<Keypair>,
                generateKeypairOnDisk,
                (KeyType),
                (override));

    MOCK_METHOD(OptRef<const Keypair>,
                findKeypair,
                (KeyType, const PublicKey &),
                (const, override));

    MOCK_METHOD(outcome::result<std::vector<PublicKey>>,
                getPublicKeys,
                (KeyType),
                (const, override));
  };

  class KeyStoreMock : public KeyStore {
   public:
    KeyStoreMock()
        : KeyStore{std::make_unique<KeySuiteStoreMock<Sr25519Provider>>(),
                   std::make_unique<KeySuiteStoreMock<Ed25519Provider>>(),
                   std::make_unique<KeySuiteStoreMock<EcdsaProvider>>(),
                   std::make_unique<KeySuiteStoreMock<BandersnatchProvider>>(),
                   std::make_shared<Ed25519ProviderMock>(),
                   std::make_shared<application::AppStateManagerMock>(),
                   KeyStore::Config{{}}},
          sr25519_{dynamic_cast<KeySuiteStoreMock<Sr25519Provider> &>(
              KeyStore ::sr25519())},
          ed25519_{dynamic_cast<KeySuiteStoreMock<Ed25519Provider> &>(
              KeyStore ::ed25519())},
          ecdsa_{dynamic_cast<KeySuiteStoreMock<EcdsaProvider> &>(
              KeyStore ::ecdsa())},
          bandersnatch_{dynamic_cast<KeySuiteStoreMock<BandersnatchProvider> &>(
              KeyStore ::bandersnatch())} {}
    ~KeyStoreMock() = default;

    KeySuiteStoreMock<Sr25519Provider> &sr25519() {
      return sr25519_;
    }

    KeySuiteStoreMock<Ed25519Provider> &ed25519() {
      return ed25519_;
    }

    KeySuiteStoreMock<EcdsaProvider> &ecdsa() {
      return ecdsa_;
    }

    KeySuiteStoreMock<BandersnatchProvider> &bandersnatch() {
      return bandersnatch_;
    }

   private:
    KeySuiteStoreMock<Sr25519Provider> &sr25519_;
    KeySuiteStoreMock<Ed25519Provider> &ed25519_;
    KeySuiteStoreMock<EcdsaProvider> &ecdsa_;
    KeySuiteStoreMock<BandersnatchProvider> &bandersnatch_;
  };
}  // namespace kagome::crypto
