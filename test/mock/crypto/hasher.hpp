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

    MOCK_CONST_METHOD1(twox_128, Hash128(gsl::span<const uint8_t>));

    MOCK_CONST_METHOD1(twox_256, Hash256(gsl::span<const uint8_t>));

    MOCK_CONST_METHOD1(blake2_256, Hash256(gsl::span<const uint8_t>));

    MOCK_CONST_METHOD1(sha2_256, Hash256(gsl::span<const uint8_t>));
  };
}  // namespace kagome::crypto

#endif  // KAGOME_TEST_MOCK_CRYPTO_HASHER_HPP
