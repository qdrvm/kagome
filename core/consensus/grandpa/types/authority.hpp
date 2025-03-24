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
    AuthorityId id;
    AuthorityWeight weight{};
    bool operator==(const Authority &other) const = default;
  };

  using Authorities =
      common::SLVector<Authority, consensus::kMaxValidatorsNumber>;

  using AuthoritySetId = uint64_t;

  /*
   * List of authorities with an identifier
   */
  struct AuthoritySet {
    AuthoritySetId id{};
    Authorities authorities;
    bool operator==(const AuthoritySet &other) const = default;
  };

}  // namespace kagome::consensus::grandpa
