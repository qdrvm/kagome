/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <openssl/evp.h>
#include <array>
#include <bit>
#include <cstdint>
#include <memory>
#include <span>

namespace kagome::crypto {
  /// https://github.com/rust-random/rand
  class RandChaCha20 {
   public:
    RandChaCha20(std::span<const uint8_t, 32> seed)
        : ctx_{EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free},
          index_{block_.size()} {
      check(EVP_EncryptInit(ctx_.get(), EVP_chacha20(), seed.data(), nullptr));
    }

    template <std::ranges::contiguous_range R>
    void shuffle(R &&items) {
      [[unlikely]] if (items.empty()) { return; }
      for (auto i = items.size() - 1; i != 0; --i) {
        std::swap(items[i], items[next(i + 1)]);
      }
    }

   private:
    void check(int r) {
      if (r != 1) {
        throw std::logic_error{"RandChaCha20"};
      }
    }

    using Block = std::array<uint32_t, 64>;
    Block block() {
      std::array<uint8_t, sizeof(Block)> zero{};
      Block r;
      int len = 0;
      check(EVP_CipherUpdate(ctx_.get(),
                             reinterpret_cast<uint8_t *>(r.data()),
                             &len,
                             zero.data(),
                             zero.size()));
      if (len != zero.size()) {
        throw std::logic_error{"RandChaCha20"};
      }
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

    std::unique_ptr<EVP_CIPHER_CTX, void (*)(EVP_CIPHER_CTX *)> ctx_;
    Block block_;
    size_t index_;
  };
}  // namespace kagome::crypto
