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

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_VOTE_CRYPTO_PROVIDER_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_VOTE_CRYPTO_PROVIDER_HPP

#include "consensus/grandpa/structs.hpp"

namespace kagome::consensus::grandpa {

  /**
   * Statelessly verifies signatures of votes and signs votes
   */
  class VoteCryptoProvider {
   public:
    virtual ~VoteCryptoProvider() = default;

    virtual bool verifyPrimaryPropose(
        const SignedPrimaryPropose &primary_propose) const = 0;
    virtual bool verifyPrevote(const SignedPrevote &prevote) const = 0;
    virtual bool verifyPrecommit(const SignedPrecommit &precommit) const = 0;

    virtual SignedPrimaryPropose signPrimaryPropose(
        const PrimaryPropose &primary_propose) const = 0;
    virtual SignedPrevote signPrevote(const Prevote &prevote) const = 0;
    virtual SignedPrecommit signPrecommit(const Precommit &precommit) const = 0;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_VOTE_CRYPTO_PROVIDER_HPP
