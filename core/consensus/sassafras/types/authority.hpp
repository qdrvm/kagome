/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/constants.hpp"
#include "crypto/bandersnatch_types.hpp"

namespace kagome::consensus::sassafras {

  using AuthorityId = crypto::BandersnatchPublicKey;

  using Authority = AuthorityId;

  using Authorities = common::SLVector<Authority, kMaxValidatorsNumber>;

  using AuthorityList = Authorities;

  using AuthorityIndex = uint32_t;

}  // namespace kagome::consensus::sassafras
