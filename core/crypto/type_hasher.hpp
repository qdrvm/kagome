/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TYPE_HASHER_HASHER_HPP_
#define KAGOME_TYPE_HASHER_HASHER_HPP_

#include <gsl/span>
#include <optional>
#include "crypto/hasher/blake2b_stream_hasher.hpp"
#include "scale/kagome_scale.hpp"

namespace kagome::crypto {

  template <typename H, typename... T>
  inline void hashTypes(H &hasher, gsl::span<uint8_t> out, T &&...t) {
    kagome::scale::encode(
        [&](const uint8_t *const val, size_t count) {
          hasher.update({val, (ssize_t)count});
        },
        std::forward<T>(t)...);

    hasher.get_final(out);
  }

  template <typename T, size_t N>
  struct Hashed {
    static_assert(N == 8 || N == 16 || N == 32 || N == 64,
                  "Unexpected hash size");
    using Type = std::decay_t<T>;
    using HashType = common::Blob<N>;

    template <typename... Args>
    Hashed(Args &&...args) : type_{std::forward<Args>(args)...} {}

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
        Blake2b_StreamHasher<N> hasher_{};
        HashType h;

        hashTypes(hasher_, {h}, type_);
        opt_hash_ = std::move(h);
      }
      return *opt_hash_;
    }

   private:
    T type_;
    mutable std::optional<HashType> opt_hash_{};
  };

}  // namespace kagome::crypto

#endif  // KAGOME_TYPE_HASHER_HASHER_HPP_
