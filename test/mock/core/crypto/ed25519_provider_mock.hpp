/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "crypto/ed25519_provider.hpp"

#include <gmock/gmock.h>

namespace kagome::crypto {
  struct Ed25519ProviderMock : public Ed25519Provider {
    MOCK_METHOD(outcome::result<Ed25519Keypair>,
                generateKeypair,
                (const Ed25519Seed &, Junctions),
                (const, override));

    MOCK_METHOD(outcome::result<Ed25519Signature>,
                sign,
                (const Ed25519Keypair &, common::BufferView),
                (const, override));

    MOCK_METHOD(outcome::result<bool>,
                verify,
                (const Ed25519Signature &,
                 common::BufferView,
                 const Ed25519PublicKey &),
                (const, override));
  };
}  // namespace kagome::crypto
