/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TEST_MOCK_CRYPTO_HASHER_HPP
#define KAGOME_TEST_MOCK_CRYPTO_HASHER_HPP

#include "crypto/hasher.hpp"

#include <gmock/gmock.h>

namespace kagome::crypto {
  class HasherMock : public Hasher {
   public:
    ~HasherMock() override = default;

    MOCK_METHOD(Hash64, twox_64, (gsl::span<const uint8_t>), (const, override));

    MOCK_METHOD(Hash128,
                blake2b_128,
                (gsl::span<const uint8_t>),
                (const, override));

    MOCK_METHOD(Hash128,
                twox_128,
                (gsl::span<const uint8_t>),
                (const, override));

    MOCK_METHOD(Hash256,
                twox_256,
                (gsl::span<const uint8_t>),
                (const, override));

    MOCK_METHOD(Hash256,
                blake2b_256,
                (gsl::span<const uint8_t>),
                (const, override));

    MOCK_METHOD(Hash256,
                blake2s_256,
                (gsl::span<const uint8_t>),
                (const, override));

    MOCK_METHOD(Hash256,
                keccak_256,
                (gsl::span<const uint8_t>),
                (const, override));

    MOCK_METHOD(Hash256,
                sha2_256,
                (gsl::span<const uint8_t>),
                (const, override));

    MOCK_METHOD(Hash512,
                blake2b_512,
                (gsl::span<const uint8_t>),
                (const, override));
  };
}  // namespace kagome::crypto

#endif  // KAGOME_TEST_MOCK_CRYPTO_HASHER_HPP
