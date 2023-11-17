/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/constants.hpp"
#include "crypto/sr25519_types.hpp"

namespace kagome::consensus::babe {

  using AuthorityId = crypto::Sr25519PublicKey;

  using AuthorityWeight = uint64_t;

  /// Authority, which participate in block production
  struct Authority {
    SCALE_TIE(2);

    AuthorityId id;
    AuthorityWeight weight{};
  };

  /// List of authorities
  using Authorities = common::SLVector<Authority, kMaxValidatorsNumber>;

  using AuthorityList = Authorities;

  using AuthorityIndex = uint32_t;

}  // namespace kagome::consensus::babe
