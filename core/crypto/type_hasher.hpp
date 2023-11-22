/**
 * Copyright Quadrivium, Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>
#include <span>
#include "crypto/hasher/blake2b_stream_hasher.hpp"
#include "scale/kagome_scale.hpp"

namespace kagome::crypto {

  template <typename H, typename... T>
  inline void hashTypes(H &hasher, common::Blob<H::kOutlen> &out, T &&...t) {
    kagome::scale::encode(
        [&](const uint8_t *const val, size_t count) {
          hasher.update({val, count});
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

    private:
    template<typename StreamHasherT>
    const HashType &getHash(StreamHasherT &hasher_) const {
      if (!opt_hash_ || StreamHasherT::ID != opt_hash_->first) {
        HashType h;
        hashTypes(hasher_, h, type_);
        opt_hash_.emplace(StreamHasherT::ID, std::move(h));
      }
      return opt_hash_->second;
    }

    public:
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

    const HashType &blake2b() const {
      Blake2b_StreamHasher<N> hasher_{};
      return getHash(hasher_);
    }

   private:
    T type_;
    mutable std::optional<std::pair<uint32_t, HashType>> opt_hash_{};
  };

  template <typename T, typename... Args>
  inline Hashed<T, 32> create256Blake(Args &&...args) {
    return Hashed<T, 32>{std::forward<Args>(args)...};
  }

}  // namespace kagome::crypto
