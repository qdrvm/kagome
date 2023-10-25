/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "macro/endianness_utils.hpp"
#include "primitives/strobe.hpp"

namespace kagome::primitives {

  template <typename T>
  inline std::array<uint8_t, sizeof(T)> decompose(T value) {
    static_assert(std::is_standard_layout_v<T> and std::is_trivial_v<T>,
                  "T must be pod!");
    static_assert(!std::is_reference_v<T>, "T must not be a reference!");

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
    if constexpr (sizeof(T) == 8) {
      value = htole64(value);
    } else if constexpr (sizeof(T) == 4) {
      value = htole32(value);
    } else if constexpr (sizeof(T) == 2) {
      value = htole16(value);
    }
#endif

    std::array<uint8_t, sizeof(T)> res;
    std::memcpy(res.data(), reinterpret_cast<uint8_t *>(&value), sizeof(T));
    return res;
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

    void initialize(libp2p::BytesIn label) {
      strobe_.initialize("Merlin v1.0"_bytes);
      append_message("dom-sep"_bytes, label);
    }

    void append_message(libp2p::BytesIn label, libp2p::BytesIn msg) {
      const uint32_t data_len = msg.size();
      strobe_.metaAd<false>(label);
      strobe_.metaAd<true>(decompose(data_len));
      strobe_.ad<false>(msg);
    }

    void append_message(libp2p::BytesIn label, const uint64_t value) {
      append_message(label, decompose(value));
    }

    /// Fill the supplied buffer with the verifier's challenge bytes.
    ///
    /// The `label` parameter is metadata about the challenge, and is
    /// also appended to the transcript.  See the [Transcript
    /// Protocols](https://merlin.cool/use/protocol.html) section of
    /// the Merlin website for details on labels.
    void challenge_bytes(libp2p::BytesIn label, std::span<uint8_t> dest) {
      const uint32_t data_len = dest.size();
      strobe_.metaAd<false>(label);
      strobe_.metaAd<true>(decompose(data_len));
      strobe_.template prf<false>(dest);
    }

    auto data() const {
      return strobe_.data();
    }

    bool operator==(const Transcript &other) const {
      return std::equal(strobe_.data().begin(),
                        strobe_.data().end(),
                        other.strobe_.data().begin(),
                        other.strobe_.data().end());
    }
  };

}  // namespace kagome::primitives
