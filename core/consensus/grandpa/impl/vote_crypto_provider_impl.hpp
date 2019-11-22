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
        const SignedPrimaryPropose &primary_propose) const override;
    bool verifyPrevote(const SignedPrevote &prevote) const override;
    bool verifyPrecommit(const SignedPrecommit &precommit) const override;

    SignedPrimaryPropose signPrimaryPropose(
        const PrimaryPropose &primary_propose) const override;
    SignedPrevote signPrevote(const Prevote &prevote) const override;
    SignedPrecommit signPrecommit(const Precommit &precommit) const override;

   private:
    template <typename VoteType>
    crypto::ED25519Signature voteSignature(uint8_t stage,
                                           const VoteType &vote_type) const;

    crypto::ED25519Keypair keypair_;
    std::shared_ptr<crypto::ED25519Provider> ed_provider_;
    RoundNumber round_number_;
    std::shared_ptr<VoterSet> voter_set_;
  };

}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CORE_CONSENSUS_GRANDPA_IMPL_VOTE_CRYPTO_PROVIDER_IMPL_HPP
