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
#include "scale/tie.hpp"

namespace kagome::primitives {

  using AuthorityWeight = uint64_t;
  using AuthorityListId = uint64_t;
  using AuthorityListSize = uint64_t;

  struct AuthorityId {
    SCALE_TIE(1);
    SCALE_TIE_EQ(AuthorityId);

    GenericSessionKey id;
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
    SCALE_TIE(2);
    SCALE_TIE_EQ(Authority);

    AuthorityId id;
    AuthorityWeight weight{};
  };

  /// Special type for vector of authorities
  struct AuthorityList : public std::vector<Authority> {
    AuthorityListId id{};

    // Attention: When adding a member, we need to ensure correct
    // destruction to avoid memory leaks or any other problem
    using std::vector<Authority>::vector;
  };

}  // namespace kagome::primitives

#endif  // KAGOME_AUTHORITY_HPP
