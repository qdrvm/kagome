/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_ROUND_STATE_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_ROUND_STATE_HPP

#include "consensus/grandpa/structs.hpp"

namespace kagome::consensus::grandpa {

  struct RoundState {
    boost::optional<Prevote>
        prevote_ghost;  // prevote that was broadcasted by current peer, which
                        // is calculated as ghost function on the last finalized
                        // block
    boost::optional<BlockInfo>
        estimate;  // best possible block that could be finalized in this round
    boost::optional<BlockInfo> finalized;  // block finalized in this round

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
