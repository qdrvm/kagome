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
    if (!primary_propose.is<PrimaryPropose>()) {
      return false;
    }
    auto payload =
        scale::encode(primary_propose.message, round_number_, voter_set_->id())
            .value();
    auto verified = ed_provider_->verify(
        primary_propose.signature, payload, primary_propose.id);
    return verified.has_value() and verified.value();
  }

  bool VoteCryptoProviderImpl::verifyPrevote(
      const SignedMessage &prevote) const {
    if (!prevote.is<Prevote>()) {
      return false;
    }
    auto payload =
        scale::encode(prevote.message, round_number_, voter_set_->id()).value();
    auto verified =
        ed_provider_->verify(prevote.signature, payload, prevote.id);
    return verified.has_value() and verified.value();
  }

  bool VoteCryptoProviderImpl::verifyPrecommit(
      const SignedMessage &precommit) const {
    if (!precommit.is<Precommit>()) {
      return false;
    }
    auto payload =
        scale::encode(precommit.message, round_number_, voter_set_->id())
            .value();
    auto verified =
        ed_provider_->verify(precommit.signature, payload, precommit.id);
    return verified.has_value() and verified.value();
  }

  crypto::ED25519Signature VoteCryptoProviderImpl::voteSignature(
      const Vote &vote) const {
    auto payload = scale::encode(vote, round_number_, voter_set_->id()).value();
    return ed_provider_->sign(keypair_, payload).value();
  }

  SignedMessage VoteCryptoProviderImpl::signPrimaryPropose(
      const PrimaryPropose &primary_propose) const {
    Vote vote(primary_propose);
    auto sign = voteSignature(vote);
    return {.message = std::move(vote),
            .signature = std::move(sign),
            .id = keypair_.public_key};
  }

  SignedMessage VoteCryptoProviderImpl::signPrevote(
      const Prevote &prevote) const {
    Vote vote(prevote);
    auto sign = voteSignature(vote);
    return {.message = std::move(vote),
            .signature = std::move(sign),
            .id = keypair_.public_key};
  }

  SignedMessage VoteCryptoProviderImpl::signPrecommit(
      const Precommit &precommit) const {
    Vote vote(precommit);
    auto sign = voteSignature(vote);
    return {.message = std::move(vote),
            .signature = std::move(sign),
            .id = keypair_.public_key};
  }
}  // namespace kagome::consensus::grandpa
