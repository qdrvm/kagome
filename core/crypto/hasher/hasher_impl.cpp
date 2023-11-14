/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/hasher/hasher_impl.hpp"

#include <span>

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

  Hash64 HasherImpl::twox_64(common::BufferView data) const {
    return make_twox64(data);
  }

  Hash64 HasherImpl::blake2b_64(common::BufferView data) const {
    Hash64 out;
    blake2b(out.data(), out.size(), nullptr, 0, data.data(), data.size());
    return out;
  }

  Hash128 HasherImpl::twox_128(common::BufferView data) const {
    return make_twox128(data);
  }

  Hash128 HasherImpl::blake2b_128(common::BufferView data) const {
    Hash128 out;
    blake2b(out.data(), out.size(), nullptr, 0, data.data(), data.size());
    return out;
  }

  Hash256 HasherImpl::twox_256(common::BufferView data) const {
    return make_twox256(data);
  }

  Hash256 HasherImpl::blake2b_256(common::BufferView data) const {
    Hash256 out;
    blake2b(out.data(), 32, nullptr, 0, data.data(), data.size());
    return out;
  }

  Hash512 HasherImpl::blake2b_512(common::BufferView data) const {
    Hash512 out;
    blake2b(out.data(), 64, nullptr, 0, data.data(), data.size());
    return out;
  }

  Hash256 HasherImpl::keccak_256(common::BufferView data) const {
    Hash256 out;
    sha3_HashBuffer(256,
                    SHA3_FLAGS::SHA3_FLAGS_KECCAK,
                    data.data(),
                    data.size(),
                    out.data(),
                    32);
    return out;
  }

  Hash256 HasherImpl::blake2s_256(common::BufferView data) const {
    Hash256 out;
    blake2s(out.data(), 32, nullptr, 0, data.data(), data.size());
    return out;
  }

  Hash256 HasherImpl::sha2_256(common::BufferView data) const {
    return crypto::sha256(data);
  }
}  // namespace kagome::crypto
