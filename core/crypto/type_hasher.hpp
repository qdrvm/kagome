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
  enum class EncodeForHashError : uint8_t {
    ALREADY_FINALIZED = 1,
  };
}
OUTCOME_HPP_DECLARE_ERROR(kagome::crypto, EncodeForHashError);
inline OUTCOME_CPP_DEFINE_CATEGORY(kagome::crypto, EncodeForHashError, e) {
  switch (e) {
    case EncodeForHashError::ALREADY_FINALIZED:
      return "Calculation of hash has already finalized before";
  }
}

namespace kagome::crypto {

  template <typename H>
  class EncoderToHash final : public scale::Encoder {
   public:
    EncoderToHash(H &hasher){};

    EncoderToHash() = default;
    EncoderToHash(EncoderToHash &&) noexcept = delete;
    EncoderToHash(const EncoderToHash &) = delete;
    EncoderToHash &operator=(EncoderToHash &&) noexcept = delete;
    EncoderToHash &operator=(const EncoderToHash &) = delete;
    void put(uint8_t byte) override {
      [[unlikely]] if (finalized_) {
        scale::raise(EncodeForHashError::ALREADY_FINALIZED);
      }
      hasher_.update({&byte, 1});
    }
    void write(std::span<const uint8_t> bytes) override {
      [[unlikely]] if (finalized_) {
        scale::raise(EncodeForHashError::ALREADY_FINALIZED);
      }
      hasher_.update(bytes);
    }
    void get_final(common::Blob<H::kOutlen> &out) & {
      [[unlikely]] if (finalized_) {
        scale::raise(EncodeForHashError::ALREADY_FINALIZED);
      }
      hasher_.get_final(out);
    }

   private:
    [[deprecated("size() is not allowed in EncoderToHash")]]  //
    [[noreturn]]                                              //
    size_t
    size() const override {
      throw std::logic_error("size() is not allowed in EncoderToHash");
    }

    H hasher_;
    bool finalized_ = false;
  };

  template <typename H, typename... T>
  inline void hashTypes(H &hasher, common::Blob<H::kOutlen> &out, T &&...t) {
    EncoderToHash<H> to_hasher{};
    // auto tie = std::tie(std::forward<T>(t)...);
    // scale::encode(tie, to_hasher);
    scale::encode(std::tie(std::forward<T>(t)...), to_hasher);
    to_hasher.get_final(out);
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
