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

#ifndef KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_ENVIRONMENT_MOCK_HPP
#define KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_ENVIRONMENT_MOCK_HPP

#include "consensus/grandpa/environment.hpp"

#include <gmock/gmock.h>

namespace kagome::consensus::grandpa {

  class EnvironmentMock : public Environment {
   public:
    MOCK_CONST_METHOD2(getAncestry,
                       outcome::result<std::vector<primitives::BlockHash>>(
                           const primitives::BlockHash &base,
                           const BlockHash &block));

    MOCK_CONST_METHOD1(bestChainContaining,
                       outcome::result<BlockInfo>(const BlockHash &base));

    MOCK_METHOD3(onProposed,
                 outcome::result<void>(RoundNumber round,
                                       MembershipCounter set_id,
                                       const SignedPrimaryPropose &propose));
    MOCK_METHOD3(onPrevoted,
                 outcome::result<void>(RoundNumber round,
                                       MembershipCounter set_id,
                                       const SignedPrevote &prevote));
    MOCK_METHOD3(onPrecommitted,
                 outcome::result<void>(RoundNumber round,
                                       MembershipCounter set_id,
                                       const SignedPrecommit &precommit));
    MOCK_METHOD3(
        onCommitted,
        outcome::result<void>(RoundNumber round,
                              const BlockInfo &vote,
                              const GrandpaJustification &justification));

    MOCK_METHOD1(doOnCompleted, void(const CompleteHandler &));

    MOCK_METHOD1(onCompleted, void(outcome::result<CompletedRound> round));

    MOCK_METHOD2(
        finalize,
        outcome::result<void>(const primitives::BlockHash &block,
                              const GrandpaJustification &justification));
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_TEST_MOCK_CORE_CONSENSUS_GRANDPA_ENVIRONMENT_MOCK_HPP
