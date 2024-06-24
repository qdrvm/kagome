/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <openssl/chacha.h>
#include <array>
#include <bit>
#include <cstdint>
#include <memory>
#include <span>

namespace kagome::crypto {
  /// https://github.com/rust-random/rand
  class RandChaCha20 {
   public:
    using Seed = std::array<uint8_t, 32>;
    RandChaCha20(const Seed &seed) : seed_{seed} {}

    template <std::ranges::contiguous_range R>
    void shuffle(R &&items) {
      [[unlikely]] if (items.empty()) { return; }
      for (auto i = items.size() - 1; i != 0; --i) {
        std::swap(items[i], items[next(i + 1)]);
      }
    }

   private:
    using Block = std::array<uint32_t, 64>;
    Block block() {
      const std::array<uint8_t, sizeof(Block)> zero{};
      Block r;
      std::array<uint8_t, 12> nonce_{};
      CRYPTO_chacha_20(reinterpret_cast<uint8_t *>(r.data()),
                       zero.data(),
                       zero.size(),
                       seed_.data(),
                       nonce_.data(),
                       counter_);

      constexpr size_t kChachaBlock = 64;
      // check BoringSSL u32 counter overflow (after generating 256GB)
      // https://github.com/pyca/cryptography/issues/8956#issuecomment-1570582021
      if (counter_ >= 0x100000000) {
        throw std::runtime_error{"RandChaCha20 overflow"};
      }
      static_assert(zero.size() % kChachaBlock == 0);
      counter_ += zero.size() / kChachaBlock;

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
      for (auto &x : r) {
        x = __builtin_bswap32(x);
      }
#endif
      return r;
    }

    uint32_t next(uint32_t n) {
      auto zone = (n << std::countl_zero(n)) - 1;
      while (true) {
        if (index_ >= block_.size()) {
          block_ = block();
          index_ = 0;
        }
        uint64_t raw = block_[index_++];
        auto mul = raw * n;
        if ((mul & 0xffffffff) <= zone) {
          return mul >> 32;
        }
      }
    }

    Seed seed_;
    uint64_t counter_ = 0;
    Block block_;
    size_t index_ = block_.size();
  };
}  // namespace kagome::crypto
