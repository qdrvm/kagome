/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_AUTHORITY_HPP
#define KAGOME_AUTHORITY_HPP

#include "consensus/babe/common.hpp"
#include "primitives/common.hpp"

namespace kagome::consensus {
  using AuthorityWeight = uint64_t;

  /**
   * Authority, which participate in Babe production
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
}  // namespace kagome::consensus

#endif  // KAGOME_AUTHORITY_HPP
