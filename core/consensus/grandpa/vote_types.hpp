/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_GRANDPA_VOTE_TYPES_HPP
#define KAGOME_CONSENSUS_GRANDPA_VOTE_TYPES_HPP

namespace kagome::consensus::grandpa {
  enum class VoteType { Prevote, Precommit };
}

#endif  // KAGOME_CONSENSUS_GRANDPA_VOTE_TYPES_HPP
