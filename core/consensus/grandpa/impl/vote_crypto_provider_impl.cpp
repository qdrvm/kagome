/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/vote_crypto_provider_impl.hpp"

#include "scale/scale.hpp"

namespace kagome::consensus::grandpa {

  VoteCryptoProviderImpl::VoteCryptoProviderImpl(
      kagome::crypto::ED25519Keypair keypair,
      std::shared_ptr<kagome::crypto::ED25519Provider> ed_provider,
      RoundNumber round_number,
      std::shared_ptr<VoterSet> voter_set)
      : keypair_{keypair},
        ed_provider_{std::move(ed_provider)},
        round_number_{round_number},
        voter_set_{std::move(voter_set)} {}

  bool VoteCryptoProviderImpl::verifyPrimaryPropose(
      const SignedMessage &primary_propose) const {
    // TODO(xDimon) change here
    auto payload = scale::encode(kPrimaryProposeStage,
                                 primary_propose.message,
                                 round_number_,
                                 voter_set_->setId())
                       .value();
    auto verified = ed_provider_->verify(
        primary_propose.signature, payload, primary_propose.id);
    return verified.has_value() and verified.value();
  }

  bool VoteCryptoProviderImpl::verifyPrevote(
      const SignedMessage &prevote) const {
    // TODO(xDimon) change here
    auto payload =
        scale::encode(
            kPrevoteStage, prevote.message, round_number_, voter_set_->setId())
            .value();
    auto verified =
        ed_provider_->verify(prevote.signature, payload, prevote.id);
    return verified.has_value() and verified.value();
  }

  bool VoteCryptoProviderImpl::verifyPrecommit(
      const SignedMessage &precommit) const {
    // TODO(xDimon) change here
    auto payload = scale::encode(kPrecommitStage,
                                 precommit.message,
                                 round_number_,
                                 voter_set_->setId())
                       .value();
    auto verified =
        ed_provider_->verify(precommit.signature, payload, precommit.id);
    return verified.has_value() and verified.value();
  }

  template <typename VoteType>
  crypto::ED25519Signature VoteCryptoProviderImpl::voteSignature(
      uint8_t stage, const VoteType &vote_type) const {
	  // TODO change here
    auto payload =
        scale::encode(vote_type, stage, round_number_, voter_set_->setId())
            .value();
    return ed_provider_->sign(keypair_, payload).value();
  }

  template crypto::ED25519Signature VoteCryptoProviderImpl::voteSignature<
      PrimaryPropose>(uint8_t, const PrimaryPropose &) const;
  template crypto::ED25519Signature
  VoteCryptoProviderImpl::voteSignature<Prevote>(uint8_t,
                                                 const Prevote &) const;
  template crypto::ED25519Signature
  VoteCryptoProviderImpl::voteSignature<Precommit>(uint8_t,
                                                   const Precommit &) const;

  SignedMessage VoteCryptoProviderImpl::signPrimaryPropose(
      const PrimaryPropose &primary_propose) const {
    return {.message = primary_propose,
            .signature = voteSignature(kPrimaryProposeStage, primary_propose),
            .id = keypair_.public_key};
  }

  SignedMessage VoteCryptoProviderImpl::signPrevote(
      const Prevote &prevote) const {
    return {.message = prevote,
            .signature = voteSignature(kPrevoteStage, prevote),
            .id = keypair_.public_key};
  }

  SignedMessage VoteCryptoProviderImpl::signPrecommit(
      const Precommit &precommit) const {
    return {.message = precommit,
            .signature = voteSignature(kPrecommitStage, precommit),
            .id = keypair_.public_key};
  }
}  // namespace kagome::consensus::grandpa
