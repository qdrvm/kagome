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

  struct Authority {
    AuthorityId id;
    AuthorityWeight weight{};
    bool operator==(const Authority &other) const = default;
  };

  using Authorities = common::SLVector<Authority, kMaxValidatorsNumber>;

}  // namespace kagome::consensus::babe
