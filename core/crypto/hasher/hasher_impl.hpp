/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "crypto/hasher.hpp"

namespace kagome::crypto {

  class HasherImpl : public Hasher {
   public:
    ~HasherImpl() override = default;

    Hash64 twox_64(common::BufferView data) const override;

    Hash64 blake2b_64(common::BufferView data) const override;

    Hash128 twox_128(common::BufferView data) const override;

    Hash128 blake2b_128(common::BufferView data) const override;

    Hash256 twox_256(common::BufferView data) const override;

    Hash256 blake2b_256(common::BufferView data) const override;

    Hash256 keccak_256(common::BufferView data) const override;

    Hash256 blake2s_256(common::BufferView data) const override;

    Hash256 sha2_256(common::BufferView data) const override;

    Hash512 blake2b_512(common::BufferView data) const override;
  };

}  // namespace kagome::crypto
