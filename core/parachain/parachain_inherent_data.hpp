/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PARACHAIN_PARACHAINSINHERENTDATA
#define KAGOME_PARACHAIN_PARACHAINSINHERENTDATA

#include "dispute_coordinator/types.hpp"
#include "network/types/collator_messages.hpp"
#include "primitives/block_header.hpp"

namespace kagome::parachain {

  struct ParachainInherentData {
    SCALE_TIE(4);

    /// The array of signed bitfields by validators claiming the candidate is
    /// available (or not).
    /// @note The array must be sorted by validator index corresponding to the
    /// authority set
    std::vector<network::SignedBitfield> bitfields;

    /// The array of backed candidates for inclusion in the current block
    std::vector<network::BackedCandidate> backed_candidates;

    /// Sets of dispute votes for inclusion,
    dispute::MultiDisputeStatementSet disputes;

    /// The head data is contains information about a parachain block. The head
    /// data is returned by  executing the parachain Runtime and relay chain
    /// validators are not concerned with its inner structure and treat it as a
    /// byte arrays.
    primitives::BlockHeader parent_header;
  };

}  // namespace kagome::parachain

#endif  // KAGOME_PARACHAIN_PARACHAINSINHERENTDATA
