/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_ROUND_STATE_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_ROUND_STATE_HPP

#include <boost/optional.hpp>

#include "consensus/grandpa/structs.hpp"

namespace kagome::consensus::grandpa {

  /// Stores the current state of the round
  struct RoundState {
    /// last finalized block before playing round
    BlockInfo last_finalized_block;

    /**
     * is calculated as ghost function on graph composed from received prevotes.
     * Note: prevote_ghost is not necessary the prevote that created by the
     * current peer
     */
    Prevote best_prevote_candidate;

    /**
     * is the best possible block that could be finalized in current round.
     * Always ancestor of `prevote_ghost` or equal to `prevote_ghost`
     */
    BlockInfo best_final_candidate;

    /**
     * is the block that received supermajority on both prevotes and precommits
     */
    boost::optional<BlockInfo> finalized;

    inline bool operator==(const RoundState &round_state) const {
      return std::tie(best_prevote_candidate, best_final_candidate, finalized)
             == std::tie(round_state.best_prevote_candidate,
                         round_state.best_final_candidate,
                         round_state.finalized);
    }

    inline bool operator!=(const RoundState &round_state) const {
      return !operator==(round_state);
    }
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const RoundState &state) {
    return s << state.best_prevote_candidate << state.best_final_candidate
             << state.finalized;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, RoundState &state) {
    return s >> state.best_prevote_candidate >> state.best_final_candidate
           >> state.finalized;
  }

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_ROUND_STATE_HPP
