/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/vote_crypto_provider_impl.hpp"

#include "primitives/common.hpp"
#include "scale/scale.hpp"

namespace kagome::consensus::grandpa {

  VoteCryptoProviderImpl::VoteCryptoProviderImpl(
      const std::shared_ptr<crypto::Ed25519Keypair> &keypair,
      std::shared_ptr<kagome::crypto::Ed25519Provider> ed_provider,
      RoundNumber round_number,
      std::shared_ptr<VoterSet> voter_set)
      : keypair_{keypair},
        ed_provider_{std::move(ed_provider)},
        round_number_{round_number},
        voter_set_{std::move(voter_set)} {}

  std::optional<SignedMessage> VoteCryptoProviderImpl::sign(Vote vote) const {
    if (not keypair_) {
      return std::nullopt;
    }
    auto payload = scale::encode(vote, round_number_, voter_set_->id()).value();
    auto signature = ed_provider_->sign(*keypair_.get(), payload).value();
    return {{.message = std::move(vote),
             .signature = signature,
             .id = keypair_->public_key}};
  }

  bool VoteCryptoProviderImpl::verify(const SignedMessage &vote,
                                      RoundNumber number) const {
    auto payload =
        scale::encode(vote.message, number, voter_set_->id()).value();
    auto verifying_result =
        ed_provider_->verify(vote.signature, payload, vote.id);
    return verifying_result.has_value() and verifying_result.value();
  }

  bool VoteCryptoProviderImpl::verifyPrimaryPropose(
      const SignedMessage &vote) const {
    return vote.is<PrimaryPropose>() and verify(vote, round_number_);
  }

  bool VoteCryptoProviderImpl::verifyPrevote(const SignedMessage &vote) const {
    return vote.is<Prevote>() and verify(vote, round_number_);
  }

  bool VoteCryptoProviderImpl::verifyPrecommit(
      const SignedMessage &vote) const {
    return vote.is<Precommit>() and verify(vote, round_number_);
  }

  std::optional<SignedMessage> VoteCryptoProviderImpl::signPrimaryPropose(
      const PrimaryPropose &primary_propose) const {
    return sign(primary_propose);
  }

  std::optional<SignedMessage> VoteCryptoProviderImpl::signPrevote(
      const Prevote &prevote) const {
    return sign(prevote);
  }

  std::optional<SignedMessage> VoteCryptoProviderImpl::signPrecommit(
      const Precommit &precommit) const {
    return sign(precommit);
  }
}  // namespace kagome::consensus::grandpa
