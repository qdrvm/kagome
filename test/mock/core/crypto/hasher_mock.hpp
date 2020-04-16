/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_TEST_MOCK_CRYPTO_HASHER_HPP
#define KAGOME_TEST_MOCK_CRYPTO_HASHER_HPP

#include "crypto/hasher.hpp"

#include <gmock/gmock.h>

namespace kagome::crypto {
  class HasherMock : public Hasher {
   public:
    ~HasherMock() override = default;

    MOCK_CONST_METHOD1(twox_64, Hash64(gsl::span<const uint8_t>));

    MOCK_CONST_METHOD1(twox_128, Hash128(gsl::span<const uint8_t>));

    MOCK_CONST_METHOD1(twox_256, Hash256(gsl::span<const uint8_t>));

    MOCK_CONST_METHOD1(blake2b_256, Hash256(gsl::span<const uint8_t>));

    MOCK_CONST_METHOD1(blake2s_256, Hash256(gsl::span<const uint8_t>));

    MOCK_CONST_METHOD1(keccak_256, Hash256(gsl::span<const uint8_t>));

    MOCK_CONST_METHOD1(sha2_256, Hash256(gsl::span<const uint8_t>));
  };
}  // namespace kagome::crypto

#endif  // KAGOME_TEST_MOCK_CRYPTO_HASHER_HPP
