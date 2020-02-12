/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CRYPTO_HASHER_HASHER_IMPL_HPP_
#define KAGOME_CORE_CRYPTO_HASHER_HASHER_IMPL_HPP_

#include "crypto/hasher.hpp"

namespace kagome::crypto {

  class HasherImpl : public Hasher {
   public:
    ~HasherImpl() override = default;

    Hash64 twox_64(gsl::span<const uint8_t> buffer) const override;

    Hash128 twox_128(gsl::span<const uint8_t> buffer) const override;

    Hash256 twox_256(gsl::span<const uint8_t> buffer) const override;

    Hash256 blake2b_256(gsl::span<const uint8_t> buffer) const override;

    Hash256 keccak_256(gsl::span<const uint8_t> buffer) const override;

    Hash256 blake2s_256(gsl::span<const uint8_t> buffer) const override;

    Hash256 sha2_256(gsl::span<const uint8_t> buffer) const override;
  };

}  // namespace kagome::hash

#endif  // KAGOME_CORE_CRYPTO_HASHER_HASHER_IMPL_HPP_
