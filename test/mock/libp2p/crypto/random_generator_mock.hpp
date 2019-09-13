/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_RANDOM_GENERATOR_MOCK_HPP
#define KAGOME_RANDOM_GENERATOR_MOCK_HPP

#include "libp2p/crypto/random_generator.hpp"

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

#endif  // KAGOME_RANDOM_GENERATOR_MOCK_HPP
