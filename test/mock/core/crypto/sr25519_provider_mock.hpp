/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>
#include "crypto/sr25519_provider.hpp"

namespace kagome::crypto {
  struct Sr25519ProviderMock : public Sr25519Provider {
    MOCK_METHOD(Sr25519Keypair,
                generateKeypair,
                (const Sr25519Seed &, Junctions),
                (const, override));

    MOCK_METHOD(outcome::result<Sr25519Signature>,
                sign,
                (const Sr25519Keypair &, std::span<const uint8_t>),
                (const, override));

    MOCK_METHOD(outcome::result<bool>,
                verify,
                (const Sr25519Signature &,
                 std::span<const uint8_t>,
                 const Sr25519PublicKey &),
                (const, override));

    MOCK_METHOD(outcome::result<bool>,
                verify_deprecated,
                (const Sr25519Signature &,
                 std::span<const uint8_t>,
                 const Sr25519PublicKey &),
                (const, override));
  };
}  // namespace kagome::crypto
