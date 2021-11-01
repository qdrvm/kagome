/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_GRANDPA_MOVABLEROUNDSTATE
#define KAGOME_CONSENSUS_GRANDPA_MOVABLEROUNDSTATE

#include <optional>

#include "consensus/grandpa/structs.hpp"

namespace kagome::consensus::grandpa {

  /// Stores the current state of the round
  struct MovableRoundState {
    RoundNumber round_number;
    BlockInfo last_finalized_block;
    std::vector<VoteVariant> votes;
    std::optional<BlockInfo> finalized;

    inline bool operator==(const MovableRoundState &round_state) const {
      return std::tie(round_number, last_finalized_block, votes, finalized)
             == std::tie(round_state.round_number,
                         round_state.last_finalized_block,
                         round_state.votes,
                         round_state.finalized);
    }

    inline bool operator!=(const MovableRoundState &round_state) const {
      return !operator==(round_state);
    }
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const MovableRoundState &state) {
    return s << state.round_number << state.last_finalized_block << state.votes
             << state.finalized;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, MovableRoundState &state) {
    return s >> state.round_number >> state.last_finalized_block >> state.votes
           >> state.finalized;
  }

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CONSENSUS_GRANDPA_MOVABLEROUNDSTATE
