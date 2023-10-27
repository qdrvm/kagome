/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "crypto/hasher.hpp"

#include <gmock/gmock.h>

namespace kagome::crypto {
  class HasherMock : public Hasher {
   public:
    ~HasherMock() override = default;

    MOCK_METHOD(Hash64, twox_64, (common::BufferView), (const, override));

    MOCK_METHOD(Hash64, blake2b_64, (common::BufferView), (const, override));

    MOCK_METHOD(Hash128, blake2b_128, (common::BufferView), (const, override));

    MOCK_METHOD(Hash128, twox_128, (common::BufferView), (const, override));

    MOCK_METHOD(Hash256, twox_256, (common::BufferView), (const, override));

    MOCK_METHOD(Hash256, blake2b_256, (common::BufferView), (const, override));

    MOCK_METHOD(Hash256, blake2s_256, (common::BufferView), (const, override));

    MOCK_METHOD(Hash256, keccak_256, (common::BufferView), (const, override));

    MOCK_METHOD(Hash256, sha2_256, (common::BufferView), (const, override));

    MOCK_METHOD(Hash512, blake2b_512, (common::BufferView), (const, override));
  };
}  // namespace kagome::crypto
