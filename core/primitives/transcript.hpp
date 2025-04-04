/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <vector>

#include "primitives/strobe.hpp"

namespace kagome::primitives {

  template <typename T>
  inline void decompose(const T &value, uint8_t (&dst)[sizeof(value)]) {
    static_assert(std::is_standard_layout_v<T> and std::is_trivial_v<T>,
                  "T must be pod!");
    static_assert(!std::is_reference_v<T>, "T must not be a reference!");

    for (size_t i = 0; i < sizeof(value); ++i) {
#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
      dst[sizeof(value) - i - 1] =
#else
      dst[i] =
#endif
          static_cast<uint8_t>((value >> (8 * i)) & 0xff);
    }
  }

  /**
   * C++ implementation of
   * https://github.com/dalek-cryptography/merlin
   */
  class Transcript final {
    Strobe strobe_;

   public:
    Transcript() = default;

    Transcript(const Transcript &) = default;
    Transcript &operator=(const Transcript &) = default;

    Transcript(Transcript &&) = delete;
    Transcript &operator=(Transcript &&) = delete;

    template <typename T, size_t N>
    void initialize(const T (&label)[N]) {
      strobe_.initialize(
          (uint8_t[11]){'M', 'e', 'r', 'l', 'i', 'n', ' ', 'v', '1', '.', '0'});
      append_message((uint8_t[]){'d', 'o', 'm', '-', 's', 'e', 'p'}, label);
    }

    template <typename T, size_t N, typename K, size_t M>
    void append_message(const T (&label)[N], const K (&msg)[M]) {
      const uint32_t data_len = sizeof(msg);
      strobe_.metaAd<false>(label);

      uint8_t tmp[sizeof(data_len)];
      decompose(data_len, tmp);

      strobe_.metaAd<true>(tmp);
      strobe_.ad<false>(msg);
    }

    template <typename T, size_t N>
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
    void append_message(const T (&label)[N], const std::vector<uint8_t> &msg) {
      const uint32_t data_len = msg.size();
      strobe_.metaAd<false>(label);

      // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
      uint8_t tmp[sizeof(data_len)];
      decompose(data_len, tmp);

      strobe_.metaAd<true>(tmp);
      strobe_.ad<false>(msg.data(), msg.size());
    }

    template <typename T, size_t N, typename IntType>
      requires std::integral<IntType>
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
    void append_message(const T (&label)[N], const IntType value) {
      // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays)
      uint8_t tmp[sizeof(value)];
      decompose(value, tmp);
      append_message(label, tmp);
    }

    /// Fill the supplied buffer with the verifier's challenge bytes.
    ///
    /// The `label` parameter is metadata about the challenge, and is
    /// also appended to the transcript.  See the [Transcript
    /// Protocols](https://merlin.cool/use/protocol.html) section of
    /// the Merlin website for details on labels.
    template <typename T, size_t N, typename K, size_t M>
    void challenge_bytes(const T (&label)[N], K (&dest)[M]) {
      const uint32_t data_len = sizeof(dest);
      strobe_.metaAd<false>(label);

      uint8_t tmp[sizeof(data_len)];
      decompose(data_len, tmp);

      strobe_.metaAd<true>(tmp);
      strobe_.template prf<false>(dest);
    }

    auto data() {
      return strobe_.data();
    }

    auto data() const {
      return strobe_.data();
    }

    bool operator==(const Transcript &other) const {
      return std::equal(other.strobe_.data().begin(),
                        other.strobe_.data().end(),
                        strobe_.data().begin());
    }
  };

}  // namespace kagome::primitives
