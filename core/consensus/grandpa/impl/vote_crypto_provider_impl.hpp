/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/grandpa/vote_crypto_provider.hpp"

namespace kagome::consensus::grandpa {
  class VoterSet;
}

namespace kagome::crypto {
  class Ed25519Provider;
}

namespace kagome::consensus::grandpa {

  class VoteCryptoProviderImpl : public VoteCryptoProvider {
   public:
    ~VoteCryptoProviderImpl() override = default;

    VoteCryptoProviderImpl(std::shared_ptr<crypto::Ed25519Keypair> keypair,
                           std::shared_ptr<crypto::Ed25519Provider> ed_provider,
                           RoundNumber round_number,
                           std::shared_ptr<VoterSet> voter_set);

    bool verifyPrimaryPropose(
        const SignedMessage &primary_propose) const override;
    bool verifyPrevote(const SignedMessage &prevote) const override;
    bool verifyPrecommit(const SignedMessage &precommit) const override;

    std::optional<SignedMessage> signPrimaryPropose(
        const PrimaryPropose &primary_propose) const override;
    std::optional<SignedMessage> signPrevote(
        const Prevote &prevote) const override;
    std::optional<SignedMessage> signPrecommit(
        const Precommit &precommit) const override;

   private:
    std::optional<SignedMessage> sign(Vote vote) const;
    bool verify(const SignedMessage &vote, RoundNumber number) const;

    std::shared_ptr<crypto::Ed25519Keypair> keypair_;
    std::shared_ptr<crypto::Ed25519Provider> ed_provider_;
    const RoundNumber round_number_;
    std::shared_ptr<VoterSet> voter_set_;
  };

}  // namespace kagome::consensus::grandpa
