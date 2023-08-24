/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_BABE_IMPL_THRESHOLD_UTIL_HPP
#define KAGOME_CORE_CONSENSUS_BABE_IMPL_THRESHOLD_UTIL_HPP

#include "consensus/timeline/types.hpp"
#include "primitives/authority.hpp"

namespace kagome::consensus::babe {

  /// Calculates the primary selection threshold for a given authority, taking
  /// into account `c` (`1 - c` represents the probability of a slot being
  /// empty).
  /// https://github.com/paritytech/substrate/blob/7010ec7716e0edf97d61a29bd0c337648b3a57ae/core/consensus/babe/src/authorship.rs#L30
  Threshold calculateThreshold(const std::pair<uint64_t, uint64_t> &ratio,
                               const primitives::AuthorityList &authorities,
                               primitives::AuthorityIndex authority_index);

}  // namespace kagome::consensus::babe

#endif  // KAGOME_CORE_CONSENSUS_BABE_IMPL_THRESHOLD_UTIL_HPP
