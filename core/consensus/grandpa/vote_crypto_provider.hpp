/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/grandpa/structs.hpp"

namespace kagome::consensus::grandpa {

  /**
   * Statelessly verifies signatures of votes and signs votes
   */
  class VoteCryptoProvider {
   public:
    virtual ~VoteCryptoProvider() = default;

    virtual bool verifyPrimaryPropose(
        const SignedMessage &primary_propose) const = 0;
    virtual bool verifyPrevote(const SignedMessage &prevote) const = 0;
    virtual bool verifyPrecommit(const SignedMessage &precommit) const = 0;

    virtual std::optional<SignedMessage> signPrimaryPropose(
        const PrimaryPropose &primary_propose) const = 0;
    virtual std::optional<SignedMessage> signPrevote(
        const Prevote &prevote) const = 0;
    virtual std::optional<SignedMessage> signPrecommit(
        const Precommit &precommit) const = 0;
  };

}  // namespace kagome::consensus::grandpa
