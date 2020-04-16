/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_ROUND_STATE_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_ROUND_STATE_HPP

#include <boost/optional.hpp>

#include "consensus/grandpa/structs.hpp"

namespace kagome::consensus::grandpa {

  /// Stores the current state of the round
  struct RoundState {
    boost::optional<Prevote>
        prevote_ghost;  //  is calculated as ghost function on graph composed
                        //  from received prevotes. Note: prevote_ghost is not
                        //  necessary the prevote that created by the current
                        //  peer
    boost::optional<BlockInfo>
        estimate;  // is the best possible block that could be finalized in
                   // current round. Always ancestor of `prevote_ghost` or equal
                   // to `prevote_ghost`
    boost::optional<BlockInfo>
        finalized;  // is the block that received supermajority on both prevotes
                    // and precommits

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
