/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_VOTE_CRYPTO_PROVIDER_IMPL_HPP
#define KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_VOTE_CRYPTO_PROVIDER_IMPL_HPP

#include "consensus/grandpa/vote_crypto_provider.hpp"
#include "consensus/grandpa/voter_set.hpp"
#include "crypto/ed25519_provider.hpp"

namespace kagome::consensus::grandpa {

  class VoteCryptoProviderImpl : public VoteCryptoProvider {
   public:
    ~VoteCryptoProviderImpl() override = default;

    VoteCryptoProviderImpl(crypto::ED25519Keypair keypair,
                           std::shared_ptr<crypto::ED25519Provider> ed_provider,
                           RoundNumber round_number,
                           std::shared_ptr<VoterSet> voter_set);

    bool verifyPrimaryPropose(
        const SignedMessage &primary_propose) const override;
    bool verifyPrevote(const SignedMessage &prevote) const override;
    bool verifyPrecommit(const SignedMessage &precommit) const override;

    SignedMessage signPrimaryPropose(
        const PrimaryPropose &primary_propose) const override;
    SignedMessage signPrevote(const Prevote &prevote) const override;
    SignedMessage signPrecommit(const Precommit &precommit) const override;

   private:
    crypto::ED25519Signature voteSignature(const Vote &vote) const;

    crypto::ED25519Keypair keypair_;
    std::shared_ptr<crypto::ED25519Provider> ed_provider_;
    RoundNumber round_number_;
    std::shared_ptr<VoterSet> voter_set_;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_VOTE_CRYPTO_PROVIDER_IMPL_HPP
