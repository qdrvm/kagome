/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STROBE_HPP
#define KAGOME_STROBE_HPP

#include <cstdint>
#include <array>
#include <cstring>
#include <type_traits>

#include "runtime/binaryen/wasm_memory_impl.hpp"
#include "primitives/math.hpp"
#include "crypto/keccak/keccak.h"

namespace kagome::primitives {

#ifndef ROTL64
#define ROTL64(x, y) (((x) << (y)) | ((x) >> (64 - (y))))
#endif

  void sha3_keccakf(uint64_t st[25]) {
    // constants
    constexpr uint64_t keccakf_rndc[24] = {
        0x0000000000000001, 0x0000000000008082, 0x800000000000808a,
        0x8000000080008000, 0x000000000000808b, 0x0000000080000001,
        0x8000000080008081, 0x8000000000008009, 0x000000000000008a,
        0x0000000000000088, 0x0000000080008009, 0x000000008000000a,
        0x000000008000808b, 0x800000000000008b, 0x8000000000008089,
        0x8000000000008003, 0x8000000000008002, 0x8000000000000080,
        0x000000000000800a, 0x800000008000000a, 0x8000000080008081,
        0x8000000000008080, 0x0000000080000001, 0x8000000080008008};
    constexpr int keccakf_rotc[24] = {1,  3,  6,  10, 15, 21, 28, 36,
                                      45, 55, 2,  14, 27, 41, 56, 8,
                                      25, 43, 62, 18, 39, 61, 20, 44};
    constexpr int keccakf_piln[24] = {10, 7,  11, 17, 18, 3,  5,  16,
                                      8,  21, 24, 4,  15, 23, 19, 13,
                                      12, 2,  20, 14, 22, 9,  6,  1};

    // variables
    int i, j, r;
    uint64_t t, bc[5];

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
    uint8_t *v;

    // endianess conversion. this is redundant on little-endian targets
    for (i = 0; i < 25; i++) {
      v = (uint8_t *)&st[i];
      st[i] = ((uint64_t)v[0]) | (((uint64_t)v[1]) << 8)
              | (((uint64_t)v[2]) << 16) | (((uint64_t)v[3]) << 24)
              | (((uint64_t)v[4]) << 32) | (((uint64_t)v[5]) << 40)
              | (((uint64_t)v[6]) << 48) | (((uint64_t)v[7]) << 56);
    }
#endif

    // actual iteration
    for (r = 0; r < 24; r++) {
      // Theta
      for (i = 0; i < 5; i++)
        bc[i] = st[i] ^ st[i + 5] ^ st[i + 10] ^ st[i + 15] ^ st[i + 20];

      for (i = 0; i < 5; i++) {
        t = bc[(i + 4) % 5] ^ ROTL64(bc[(i + 1) % 5], 1);
        for (j = 0; j < 25; j += 5) st[j + i] ^= t;
      }

      // Rho Pi
      t = st[1];
      for (i = 0; i < 24; i++) {
        j = keccakf_piln[i];
        bc[0] = st[j];
        st[j] = ROTL64(t, keccakf_rotc[i]);
        t = bc[0];
      }

      //  Chi
      for (j = 0; j < 25; j += 5) {
        for (i = 0; i < 5; i++) bc[i] = st[j + i];
        for (i = 0; i < 5; i++)
          st[j + i] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
      }

      //  Iota
      st[0] ^= keccakf_rndc[r];
    }

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
    // endianess conversion. this is redundant on little-endian targets
    for (i = 0; i < 25; i++) {
      v = (uint8_t *)&st[i];
      t = st[i];
      v[0] = t & 0xFF;
      v[1] = (t >> 8) & 0xFF;
      v[2] = (t >> 16) & 0xFF;
      v[3] = (t >> 24) & 0xFF;
      v[4] = (t >> 32) & 0xFF;
      v[5] = (t >> 40) & 0xFF;
      v[6] = (t >> 48) & 0xFF;
      v[7] = (t >> 56) & 0xFF;
    }
#endif
  }

  class Strobe final {
    static constexpr size_t kBufferSize = 200ull;
    static constexpr size_t kAlignment = 8ull;
    static constexpr uint8_t kStrobeR = 166;

    using Flags = uint8_t;
    using Position = uint8_t;

    static constexpr Flags kFlag_NU = 0x00;  // NU = No Use
    static constexpr Flags kFlag_I = 0x01;
    static constexpr Flags kFlag_A = 0x02;
    static constexpr Flags kFlag_C = 0x04;
    static constexpr Flags kFlag_T = 0x08;
    static constexpr Flags kFlag_M = 0x10;
    static constexpr Flags kFlag_K = 0x20;

    Flags current_state_;
    Position current_position_;
    Position begin_position_;

    uint8_t raw_data[kBufferSize + kAlignment - 1ull];
    uint8_t *const buffer_;

    template <typename T, size_t kOffset = 0ull>
    constexpr T *as() {
      static_assert(kOffset < count<T>(), "Overflow!");
      return (reinterpret_cast<T *>(buffer_) + kOffset);  // NOLINT
    }

    template <typename T>
    T *as(Position offset) {
      BOOST_ASSERT(static_cast<size_t>(offset) < count<T>());
      return (as<T>() + offset);
    }

    template <typename T>
    static constexpr size_t count() {
      static_assert(!std::is_reference_v<T>, "Can not be a reference type.");
      static_assert(!std::is_pointer_v<T>, "Can not be a pointer.");
      static_assert(std::is_integral_v<T>, "Cast to non integral type.");
      return kBufferSize / sizeof(T);
    }

    template <size_t kOffset, typename T, size_t N>
    void load(const T (&data)[N]) {  // NOLINT
      static_assert(kOffset + N <= count<T>(), "Load overflows!");
      std::memcpy(as<T, kOffset>(), data, N);
    }

    template <typename T, size_t N>
    void absorb(const T (&src)[N]) {
      for (const auto i : src) {
        *as<uint8_t>(current_position_++) ^= static_cast<uint8_t>(i);
        if (kStrobeR == current_position_) {
          // TODO: run_f();
        }
      }
    }

    template <bool kMore, Flags kFlags>
    void beginOp() {
      static_assert((kFlags & kFlag_T) == 0, "T flag doesn't support");
      if constexpr (kMore) {
        BOOST_ASSERT(current_state_ == kFlags);
        return;
      }

      const auto old_begin = begin_position_;
      begin_position_ = current_position_ + 1;
      current_state_ = kFlags;
      absorb({old_begin, kFlags});

      if constexpr (0 != (kFlags & (kFlag_C | kFlag_K))) {
        if (current_position_ != 0) {
          // TODO: run_f();
        }
      }
    }

    template <bool kMore, typename T, size_t N>
    void metaAd(const T (&label)[N]) {
      beginOp<kMore, kFlag_M | kFlag_A>();
      absorb(label);
    }
    /*
        fn run_f(&mut self) {
          self.state[self.pos as usize] ^= self.pos_begin;
          self.state[(self.pos + 1) as usize] ^= 0x04;
          self.state[(STROBE_R + 1) as usize] ^= 0x80;
          keccak::f1600(transmute_state(&mut self.state));
          self.pos = 0;
          self.pos_begin = 0;
        }*/

   public:
    Strobe()
        : buffer_{reinterpret_cast<uint8_t *>(
            math::roundUp<kAlignment>(reinterpret_cast<uintptr_t>(raw_data)))} {
    }

    template <typename T, size_t N>
    void initialize(const T (&label)[N]) {
      constexpr bool kUseRuntimeCalculation = false;

      if constexpr (kUseRuntimeCalculation) {
        std::memset(as<uint8_t>(), 0, count<uint8_t>());
        load<0ull>((uint8_t[6]){1, kStrobeR + 2, 1, 0, 1, 96});
        load<6ull>("STROBEv1.0.2");
        sha3_keccakf(as<uint64_t>());
      } else {
        load<0ull>((uint8_t[kBufferSize]){
            0x9c, 0x6d, 0x16, 0x8f, 0xf8, 0xfd, 0x55, 0xda, 0x2a, 0xa7, 0x3c,
            0x23, 0x55, 0x65, 0x35, 0x63, 0xdc, 0xc,  0x47, 0x5c, 0x55, 0x15,
            0x26, 0xf6, 0x73, 0x3b, 0xea, 0x22, 0xf1, 0x6c, 0xb5, 0x7c, 0xd3,
            0x1f, 0x68, 0x2e, 0x66, 0xe,  0xe9, 0x12, 0x82, 0x4a, 0x77, 0x22,
            0x1,  0xee, 0x13, 0x94, 0x22, 0x6f, 0x4a, 0xfc, 0xb6, 0x2d, 0x33,
            0x12, 0x93, 0xcc, 0x92, 0xe8, 0xa6, 0x24, 0xac, 0xf6, 0xe1, 0xb6,
            0x0,  0x95, 0xe3, 0x22, 0xbb, 0xfb, 0xc8, 0x45, 0xe5, 0xb2, 0x69,
            0x95, 0xfe, 0x7d, 0x7c, 0x84, 0x13, 0x74, 0xd1, 0xff, 0x58, 0x98,
            0xc9, 0x2e, 0xe0, 0x63, 0x6b, 0x6,  0x72, 0x73, 0x21, 0xc9, 0x2a,
            0x60, 0x39, 0x7,  0x3,  0x53, 0x49, 0xcc, 0xbb, 0x1b, 0x92, 0xb7,
            0xb0, 0x5,  0x7e, 0x8f, 0xa8, 0x7f, 0xce, 0xbc, 0x7e, 0x88, 0x65,
            0x6f, 0xcb, 0x45, 0xae, 0x4,  0xbc, 0x34, 0xca, 0xbe, 0xae, 0xbe,
            0x79, 0xd9, 0x17, 0x50, 0xc0, 0xe8, 0xbf, 0x13, 0xb9, 0x66, 0x50,
            0x4d, 0x13, 0x43, 0x59, 0x72, 0x65, 0xdd, 0x88, 0x65, 0xad, 0xf9,
            0x14, 0x9,  0xcc, 0x9b, 0x20, 0xd5, 0xf4, 0x74, 0x44, 0x4,  0x1f,
            0x97, 0xb6, 0x99, 0xdd, 0xfb, 0xde, 0xe9, 0x1e, 0xa8, 0x7b, 0xd0,
            0x9b, 0xf8, 0xb0, 0x2d, 0xa7, 0x5a, 0x96, 0xe9, 0x47, 0xf0, 0x7f,
            0x5b, 0x65, 0xbb, 0x4e, 0x6e, 0xfe, 0xfa, 0xa1, 0x6a, 0xbf, 0xd9,
            0xfb, 0xf6});
      }

      current_position_ = 0;
      current_state_ = kFlag_NU;
      begin_position_ = 0;

      metaAd<false>(label);
    }
  };

}

#endif//KAGOME_STROBE_HPP
