/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
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
  struct AuthorityIndex {
    uint64_t index;

    bool operator==(const AuthorityIndex &other) const {
      return index == other.index;
    }
    bool operator!=(const AuthorityIndex &other) const {
      return !(*this == other);
    };
  };

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
   * @brief outputs object of type AuthorityIndex to stream
   * @tparam Stream output stream type
   * @param s stream reference
   * @param v value to output
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const AuthorityIndex &a) {
    return s << a.index;
  }

  /**
   * @brief decodes object of type AuthorityIndex from stream
   * @tparam Stream input stream type
   * @param s stream reference
   * @param v value to decode
   * @return reference to stream
   */
  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, AuthorityIndex &a) {
    return s >> a.index;
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

namespace std {
  template <>
  struct hash<kagome::primitives::AuthorityIndex> {
    size_t operator()(const kagome::primitives::AuthorityIndex &x) const {
      return boost::hash_value(x.index);
    }
  };
}  // namespace std

#endif  // KAGOME_AUTHORITY_HPP
