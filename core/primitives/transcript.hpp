/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_TRANSACRIPT_HPP
#define KAGOME_TRANSACRIPT_HPP

#include "primitives/strobe.hpp"

namespace kagome::primitives {

  class Transcript final {
    Transcript(const Transcript &) = delete;
    Transcript &operator=(const Transcript &) = delete;

    Transcript(Transcript &&) = delete;
    Transcript &operator=(Transcript &&) = delete;

    Strobe strobe_;

   public:
    Transcript() {}

    template <typename T, size_t N>
    void initialize(const T (&label)[N]) {
      strobe_.initialize(
          (uint8_t[11]){'M', 'e', 'r', 'l', 'i', 'n', ' ', 'v', '1', '.', '0'});
      append_message((uint8_t[]){'d','o','m','-','s','e','p'}, label);
    }

    template <typename T, size_t N, typename K, size_t M>
    void append_message(const T (&label)[N], const K (&msg)[M]) {
      /// TODO(iceseer): may be data_len should be BE-LE convertable
      const uint32_t data_len = sizeof(msg);
      strobe_.metaAd<false>(label);
      strobe_.metaAd<true>((uint8_t[4]){data_len & 0xff,
                                        (data_len >> 8) & 0xff,
                                        (data_len >> 16) & 0xff,
                                        (data_len >> 24) & 0xff});
      strobe_.ad<false>(msg);
    }

    template <typename T, size_t N>
    void append_message(const T (&label)[N], const uint64_t value) {
      /// TODO(iceseer): may be data_len should be BE-LE convertable
      append_message(label,
                     (uint8_t[8]){static_cast<uint8_t>(value & 0xff),
                                  static_cast<uint8_t>((value >> 8) & 0xff),
                                  static_cast<uint8_t>((value >> 16) & 0xff),
                                  static_cast<uint8_t>((value >> 24) & 0xff),
                                  static_cast<uint8_t>((value >> 32) & 0xff),
                                  static_cast<uint8_t>((value >> 40) & 0xff),
                                  static_cast<uint8_t>((value >> 48) & 0xff),
                                  static_cast<uint8_t>((value >> 56) & 0xff)});
    }
  };

}

#endif//KAGOME_TRANSACRIPT_HPP