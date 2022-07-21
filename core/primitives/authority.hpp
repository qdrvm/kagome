/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_AUTHORITY_HPP
#define KAGOME_AUTHORITY_HPP

#include <cstdint>
#include <functional>

#include "primitives/common.hpp"
#include "primitives/session_key.hpp"

namespace kagome::primitives {

  using AuthorityWeight = uint64_t;
  using AuthoritySetId = uint64_t;
  using AuthorityListSize = uint64_t;

  struct AuthorityId {
    GenericSessionKey id;

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
  using AuthorityIndex = uint32_t;

  /**
   * Authority, which participate in block production and finalization
   */
  struct Authority {
    AuthorityId id;
    AuthorityWeight weight{};

    bool operator==(const Authority &other) const {
      return id == other.id && weight == other.weight;
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
    return s << a.id << a.weight;
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
    return s >> a.id >> a.weight;
  }

  /**
   * List of authorities
   */
  using AuthorityList = std::vector<Authority>;

  /*
   * List of authorities with an identifier
   */
  struct AuthoritySet {
    AuthoritySet() = default;

    AuthoritySet(AuthoritySetId id, AuthorityList authorities)
        : id{id}, authorities{authorities} {}

    AuthoritySetId id{};
    AuthorityList authorities;

    auto begin() {
      return authorities.begin();
    }

    auto end() {
      return authorities.end();
    }

    auto begin() const {
      return authorities.cbegin();
    }

    auto end() const {
      return authorities.cend();
    }
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, AuthoritySet &a) {
    return s >> a.id >> a.authorities;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const AuthoritySet &a) {
    return s << a.id << a.authorities;
  }

}  // namespace kagome::primitives

#endif  // KAGOME_AUTHORITY_HPP
