/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
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
