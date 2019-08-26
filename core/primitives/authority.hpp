/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_AUTHORITY_HPP
#define KAGOME_AUTHORITY_HPP

#include <cstdint>

#include "common.hpp"

namespace kagome::primitives {
  using AuthorityWeight = uint64_t;

  /**
   * Authority, which participate in block production and finalization
   */
  struct Authority {
    primitives::AuthorityId id;
    AuthorityWeight babe_weight{};

    bool operator==(const Authority &other) const {
      return id == other.id && babe_weight == other.babe_weight;
    }
    bool operator!=(const Authority &other) const {
      return !(*this == other);
    }
  };

  /**
   * @brief outputs object of type Authority to stream
   * @tparam Stream output stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const Authority &a) {
    return s << a.id << a.babe_weight;
  }

  /**
   * @brief decodes object of type Authority from stream
   * @tparam Stream input stream type
   * @param s stream reference
   * @param v value to decode
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, Authority &a) {
    return s >> a.id >> a.babe_weight;
  }
}  // namespace kagome::primitives

#endif  // KAGOME_AUTHORITY_HPP
