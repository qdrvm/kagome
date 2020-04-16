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

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_COMPLETED_ROUND_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_COMPLETED_ROUND_HPP

#include "consensus/grandpa/round_state.hpp"

namespace kagome::consensus::grandpa {

  // Structure containing state and number of completed round. Used to start new
  // round
  struct CompletedRound {
    RoundNumber round_number{};
    RoundState state;

    bool operator==(const CompletedRound &rhs) const {
      return round_number == rhs.round_number and state == rhs.state;
    }
    bool operator!=(const CompletedRound &rhs) const {
      return !operator==(rhs);
    }
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const CompletedRound &round) {
    return s << round.round_number << round.state;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, CompletedRound &round) {
    return s >> round.round_number >> round.state;
  }

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_COMPLETED_ROUND_HPP
