/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_COMMON_HPP
#define KAGOME_COMMON_HPP

#include <cstdint>

#include "crypto/vrf_types.hpp"
#include "primitives/common.hpp"

namespace kagome::consensus {
  using AuthorityWeight = uint64_t;
  struct Authority {
    primitives::AuthorityId id;
    AuthorityWeight babe_weight;
  };

  using BabeSlotNumber = uint64_t;

  /// threshold, which must not be exceeded for the party to be a slot leader
  using Threshold = crypto::VRFValue;

  // TODO(akvinikym) should be a VRFValue?
  using Randomness = boost::multiprecision::uint256_t;
}  // namespace kagome::consensus

#endif  // KAGOME_COMMON_HPP
