/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "crypto/pbkdf2/pbkdf2_provider.hpp"

namespace kagome::crypto {

  class Pbkdf2ProviderImpl : public Pbkdf2Provider {
   public:
    ~Pbkdf2ProviderImpl() override = default;

    outcome::result<common::Buffer> deriveKey(std::span<const uint8_t> data,
                                              std::span<const uint8_t> salt,
                                              size_t iterations,
                                              size_t key_length) const override;
  };

}  // namespace kagome::crypto
