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

    outcome::result<SecureBuffer<>> deriveKey(common::BufferView data,
                                              common::BufferView salt,
                                              size_t iterations,
                                              size_t key_length) const override;
  };

}  // namespace kagome::crypto
