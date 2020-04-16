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
