/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "crypto/key_store.hpp"

#include <gmock/gmock.h>

namespace kagome::crypto {
  
  template<Suite T>
  class KeySuiteStoreMock : public KeySuiteStore<T> {
  public:
    MOCK_METHOD(outcome::result<Keypair>,
                generateKeypair,
                (KeyType, std::string_view),
                (override));

    MOCK_METHOD(outcome::result<Keypair>,
                generateKeypair,
                (KeyType, common::BufferView)
                (override));

    MOCK_METHOD(outcome::result<Keypair>,
                generateKeypairOnDisk,
                (KeyType),
                (override));

    MOCK_METHOD(outcome::result<Keypair>,
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
     KeyStoreMock(): KeyStore {}
    ~KeyStoreMock() override = default;

  };
}  // namespace kagome::crypto
