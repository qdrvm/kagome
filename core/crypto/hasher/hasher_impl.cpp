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

#include "crypto/hasher/hasher_impl.hpp"

#include <gsl/span>

#include "crypto/blake2/blake2b.h"
#include "crypto/blake2/blake2s.h"
#include "crypto/keccak/keccak.h"
#include "crypto/sha/sha256.hpp"
#include "crypto/twox/twox.hpp"

namespace kagome::crypto {

  HasherImpl::Hash64 HasherImpl::twox_64(
      gsl::span<const uint8_t> buffer) const {
    return make_twox64(buffer);
  }

  HasherImpl::Hash128 HasherImpl::twox_128(
      gsl::span<const uint8_t> buffer) const {
    return make_twox128(buffer);
  }

  HasherImpl::Hash256 HasherImpl::twox_256(
      gsl::span<const uint8_t> buffer) const {
    return make_twox256(buffer);
  }

  HasherImpl::Hash256 HasherImpl::blake2b_256(
      gsl::span<const uint8_t> buffer) const {
    Hash256 out;
    blake2b(out.data(), 32, nullptr, 0, buffer.data(), buffer.size());
    return out;
  }

  HasherImpl::Hash256 HasherImpl::keccak_256(
      gsl::span<const uint8_t> buffer) const {
    Hash256 out;
    sha3_HashBuffer(256,
                    SHA3_FLAGS::SHA3_FLAGS_KECCAK,
                    buffer.data(),
                    buffer.size(),
                    out.data(),
                    32);
    return out;
  }

  HasherImpl::Hash256 HasherImpl::blake2s_256(
      gsl::span<const uint8_t> buffer) const {
    Hash256 out;
    blake2s(out.data(), 32, nullptr, 0, buffer.data(), buffer.size());
    return out;
  }

  HasherImpl::Hash256 HasherImpl::sha2_256(
      gsl::span<const uint8_t> buffer) const {
    return crypto::sha256(buffer);
  }
}  // namespace kagome::crypto
