/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_AUTHORITY_HPP
#define KAGOME_AUTHORITY_HPP

#include <cstdint>
#include <functional>

#include "primitives/common.hpp"

namespace kagome::primitives {
  using AuthorityWeight = uint64_t;

  /**
   * Authority id
   */
  struct AuthorityId {
    // TODO(kamilsa): id types should be different for Babe and Grandpa
    // Authority Ids
    SessionKey id;

    bool operator==(const AuthorityId &other) const {
      return id == other.id;
    }
    bool operator!=(const AuthorityId &other) const {
      return !(*this == other);
    }
  };

  inline bool operator<(const AuthorityId &lhs, const AuthorityId &rhs) {
    return lhs.id < rhs.id;
  }

  /**
   * Authority index
   */
  using AuthorityIndex = uint64_t;

  /**
   * Authority, which participate in block production and finalization
   */
  struct Authority {
    AuthorityId id;
    AuthorityWeight babe_weight{};

    bool operator==(const Authority &other) const {
      return id == other.id && babe_weight == other.babe_weight;
    }
    bool operator!=(const Authority &other) const {
      return !(*this == other);
    }
  };

  /**
   * @brief outputs object of type AuthorityId to stream
   * @tparam Stream output stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const AuthorityId &a) {
    return s << a.id;
  }

  /**
   * @brief decodes object of type AuthorityId from stream
   * @tparam Stream input stream type
   * @param s stream reference
   * @param v value to decode
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, AuthorityId &a) {
    return s >> a.id;
  }

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
