/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_ROUND_STATE_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_ROUND_STATE_HPP

#include "consensus/grandpa/structs.hpp"

namespace kagome::consensus::grandpa {

  struct RoundState {
    boost::optional<Prevote> prevote_ghost;
    boost::optional<BlockInfo> estimate;
    boost::optional<BlockInfo> finalized;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_ROUND_STATE_HPP
