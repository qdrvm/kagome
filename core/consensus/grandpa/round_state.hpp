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
    BlockInfo last_finalized_block{};

    /**
     * is calculated as ghost function on graph composed from received prevotes.
     * Note: prevote_ghost is not necessary the prevote that created by the
     * current peer
     */
    boost::optional<Prevote> prevote_ghost;

    /**
     * is the best possible block that could be finalized in current round.
     * Always ancestor of `prevote_ghost` or equal to `prevote_ghost`
     */
    boost::optional<BlockInfo> estimate;

    /**
     * is the block that received supermajority on both prevotes and precommits
     */
    boost::optional<BlockInfo> finalized;

    inline bool operator==(const RoundState &round_state) const {
      return std::tie(prevote_ghost, estimate, finalized)
             == std::tie(round_state.prevote_ghost,
                         round_state.estimate,
                         round_state.finalized);
    }

    inline bool operator!=(const RoundState &round_state) const {
      return !operator==(round_state);
    }
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const RoundState &state) {
    return s << state.prevote_ghost << state.estimate << state.finalized;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, RoundState &state) {
    return s >> state.prevote_ghost >> state.estimate >> state.finalized;
  }

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_ROUND_STATE_HPP
