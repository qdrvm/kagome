/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/hasher/hasher_impl.hpp"

#include <gsl/span>

#include "crypto/blake2/blake2b.h"
#include "crypto/blake2/blake2s.h"
#include "crypto/keccak/keccak.h"
#include "crypto/sha/sha256.hpp"
#include "crypto/twox/twox.hpp"

namespace kagome::crypto {
  using common::Hash128;
  using common::Hash256;
  using common::Hash512;
  using common::Hash64;

  Hash64 HasherImpl::twox_64(gsl::span<const uint8_t> buffer) const {
    return make_twox64(buffer);
  }

  Hash128 HasherImpl::twox_128(gsl::span<const uint8_t> buffer) const {
    return make_twox128(buffer);
  }

  Hash128 HasherImpl::blake2b_128(gsl::span<const uint8_t> buffer) const {
    Hash128 out;
    blake2b(out.data(), out.size(), nullptr, 0, buffer.data(), buffer.size());
    return out;
  }

  Hash256 HasherImpl::twox_256(gsl::span<const uint8_t> buffer) const {
    return make_twox256(buffer);
  }

  Hash256 HasherImpl::blake2b_256(gsl::span<const uint8_t> buffer) const {
    Hash256 out;
    blake2b(out.data(), 32, nullptr, 0, buffer.data(), buffer.size());
    return out;
  }

  Hash512 HasherImpl::blake2b_512(gsl::span<const uint8_t> buffer) const {
    Hash512 out;
    blake2b(out.data(), 64, nullptr, 0, buffer.data(), buffer.size());
    return out;
  }

  Hash256 HasherImpl::keccak_256(gsl::span<const uint8_t> buffer) const {
    Hash256 out;
    sha3_HashBuffer(256,
                    SHA3_FLAGS::SHA3_FLAGS_KECCAK,
                    buffer.data(),
                    buffer.size(),
                    out.data(),
                    32);
    return out;
  }

  Hash256 HasherImpl::blake2s_256(gsl::span<const uint8_t> buffer) const {
    Hash256 out;
    blake2s(out.data(), 32, nullptr, 0, buffer.data(), buffer.size());
    return out;
  }

  Hash256 HasherImpl::sha2_256(gsl::span<const uint8_t> buffer) const {
    return crypto::sha256(buffer);
  }
}  // namespace kagome::crypto
