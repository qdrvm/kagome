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

#ifndef LIBP2P_RANDOM_GENERATOR_MOCK_HPP
#define LIBP2P_RANDOM_GENERATOR_MOCK_HPP

#include <libp2p/crypto/random_generator.hpp>

#include <gmock/gmock.h>

namespace libp2p::crypto::random {
  struct RandomGeneratorMock : public RandomGenerator {
    ~RandomGeneratorMock() override = default;

    MOCK_METHOD1(randomBytes, std::vector<uint8_t>(size_t));
  };

  struct CSPRNGMock : public CSPRNG {
    ~CSPRNGMock() override = default;

    MOCK_METHOD1(randomBytes, std::vector<uint8_t>(size_t));
  };
}  // namespace libp2p::crypto::random

#endif  // LIBP2P_RANDOM_GENERATOR_MOCK_HPP
