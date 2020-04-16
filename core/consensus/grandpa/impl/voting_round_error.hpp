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

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_VOTING_ROUND_ERROR_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_VOTING_ROUND_ERROR_HPP

#include <outcome/outcome.hpp>

namespace kagome::consensus::grandpa {

  enum class VotingRoundError {
    FIN_VALIDATION_FAILED = 1,
    LAST_ESTIMATE_BETTER_THAN_PREVOTE,
    NEW_STATE_EQUAL_TO_OLD
  };

}

OUTCOME_HPP_DECLARE_ERROR(kagome::consensus::grandpa, VotingRoundError);

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_VOTING_ROUND_ERROR_HPP
