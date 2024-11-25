/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>

#include "crypto/hasher/blake2b_stream_hasher.hpp"
#include "scale/kagome_scale.hpp"

namespace kagome::crypto {

  template <typename H, typename... T>
  inline void hashTypes(H &hasher, common::Blob<H::kOutlen> &out, T &&...t) {
    scale::encode([&](const uint8_t *const ptr, size_t count) {
      hasher.update(std::span<const uint8_t>(ptr, count));
    }, std::forward<T>(t)...).value();
    hasher.get_final(out);
  }

  template <typename T, size_t N, typename StreamHasherT>
  struct Hashed {
    static_assert(N == 8 || N == 16 || N == 32 || N == 64,
                  "Unexpected hash size");
    using Type = T;
    using HashType = common::Blob<N>;

   public:
    // NOLINTNEXTLINE(google-explicit-constructor)
    Hashed(Type &&type)
      requires(std::is_same_v<T, std::decay_t<T>>)
        : type_{std::move(type)} {}
    // NOLINTNEXTLINE(google-explicit-constructor)
    Hashed(const Type &type) : type_{type} {}

    Hashed(const Hashed &c) = default;
    Hashed(Hashed &&c) = default;

    Hashed &operator=(const Hashed &c) = default;
    Hashed &operator=(Hashed &&c) = default;

    const Type &get() const {
      return type_;
    }

    const HashType *operator->() {
      return &type_;
    }

    Type &get_mut() {
      opt_hash_ = std::nullopt;
      return type_;
    }

    const HashType &getHash() const {
      if (!opt_hash_) {
        HashType h;
        StreamHasherT hasher_{};
        hashTypes(hasher_, h, type_);
        opt_hash_ = std::move(h);
      }
      return *opt_hash_;
    }
#ifndef CFG_TESTING
   private:
#endif  // CFG_TESTING
    T type_;
    mutable std::optional<HashType> opt_hash_{};
  };

  template <typename T, typename... Args>
  inline Hashed<T, 32, Blake2b_StreamHasher<32>> create256Blake(
      Args &&...args) {
    return Hashed<T, 32, Blake2b_StreamHasher<32>>{std::forward<Args>(args)...};
  }

}  // namespace kagome::crypto
