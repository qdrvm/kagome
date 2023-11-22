/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/constants.hpp"
#include "crypto/ed25519_types.hpp"

namespace kagome::consensus::grandpa {

  using AuthorityId = crypto::Ed25519PublicKey;

  using AuthorityWeight = uint64_t;

  struct Authority {
    SCALE_TIE(2);

    AuthorityId id;
    AuthorityWeight weight{};
  };

  using Authorities =
      common::SLVector<Authority, consensus::kMaxValidatorsNumber>;

  using AuthorityIndex = uint32_t;

  using AuthoritySetId = uint64_t;

  /*
   * List of authorities with an identifier
   */
  struct AuthoritySet {
    SCALE_TIE(2);

    AuthoritySet() = default;

    AuthoritySet(AuthoritySetId id, Authorities authorities)
        : id{id}, authorities{authorities} {}

    AuthoritySetId id{};
    Authorities authorities;
  };

}  // namespace kagome::consensus::grandpa
