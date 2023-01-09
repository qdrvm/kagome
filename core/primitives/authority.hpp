/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_AUTHORITY_HPP
#define KAGOME_AUTHORITY_HPP

#include <cstdint>
#include <functional>

#include "consensus/constants.hpp"
#include "primitives/common.hpp"
#include "primitives/session_key.hpp"
#include "scale/tie.hpp"

namespace kagome::primitives {

  using AuthorityWeight = uint64_t;
  using AuthoritySetId = uint64_t;
  using AuthorityListSize = uint64_t;

  struct AuthorityId {
    SCALE_TIE(1);

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

    AuthorityId id;
    AuthorityWeight weight{};
  };

  /**
   * List of authorities
   */
  using AuthorityList =
      common::SLVector<Authority, consensus::kMaxValidatorsNumber>;

  /*
   * List of authorities with an identifier
   */
  struct AuthoritySet {
    SCALE_TIE(2);

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

}  // namespace kagome::primitives

#endif  // KAGOME_AUTHORITY_HPP
