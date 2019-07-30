/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_COMMON_HPP
#define KAGOME_COMMON_HPP

#include <cstdint>

#include "primitives/common.hpp"

namespace kagome::consensus {
  using AuthorityWeight = uint64_t;
  struct Authority {
    primitives::AuthorityId id;
    AuthorityWeight babe_weight;
  };

  using BabeSlotNumber = uint64_t;
}  // namespace kagome::consensus

#endif  // KAGOME_COMMON_HPP
