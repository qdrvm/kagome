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
