/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <map>
#include <set>

#include "runtime/runtime_api/parachain_host_types.hpp"

namespace kagome::parachain {
  /// The claim queue mapped by parachain id.
  using TransposedClaimQueue =
      std::map<ParachainId, std::map<uint8_t, std::set<CoreIndex>>>;

  /// Returns a mapping between the para id and the core indices assigned at
  /// different
  /// depths in the claim queue.
  inline TransposedClaimQueue transposeClaimQueue(
      const runtime::ClaimQueueSnapshot &claims) {
    TransposedClaimQueue r;
    for (auto &[core, paras] : claims.claimes) {
      size_t depth = 0;
      for (auto &para : paras) {
        r[para][depth].emplace(core);
        ++depth;
      }
    }
    return r;
  }
}  // namespace kagome::parachain
