/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/validator/signer.hpp"

namespace kagome::parachain {
  outcome::result<SigningContext> SigningContext::make(
      const std::shared_ptr<runtime::ParachainHost> &parachain_api,
      const primitives::BlockHash &relay_parent) {
    OUTCOME_TRY(session_index,
                parachain_api->session_index_for_child(relay_parent));
    return SigningContext{.session_index = session_index,
                          .relay_parent = relay_parent};
  }

  ValidatorSigner::ValidatorSigner(
      ValidatorIndex validator_index,
      SigningContext context,
      std::shared_ptr<crypto::Sr25519Keypair> keypair,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<crypto::Sr25519Provider> sr25519_provider)
      : validator_index_{validator_index},
        context_{context},
        keypair_{std::move(keypair)},
        hasher_{std::move(hasher)},
        sr25519_provider_{std::move(sr25519_provider)} {}

  ValidatorSigner::ValidatorIndex ValidatorSigner::validatorIndex() const {
    return validator_index_;
  }

  SessionIndex ValidatorSigner::getSessionIndex() const {
    return context_.session_index;
  }

  const primitives::BlockHash &ValidatorSigner::relayParent() const {
    return context_.relay_parent;
  }

  ValidatorSignerFactory::ValidatorSignerFactory(
      std::shared_ptr<runtime::ParachainHost> parachain_api,
      std::shared_ptr<crypto::SessionKeys> session_keys,
      std::shared_ptr<crypto::Hasher> hasher,
      std::shared_ptr<crypto::Sr25519Provider> sr25519_provider)
      : parachain_api_{std::move(parachain_api)},
        session_keys_{std::move(session_keys)},
        hasher_{std::move(hasher)},
        sr25519_provider_{std::move(sr25519_provider)} {}

  outcome::result<std::optional<ValidatorSigner>> ValidatorSignerFactory::at(
      const primitives::BlockHash &relay_parent) {
    OUTCOME_TRY(validators, parachain_api_->validators(relay_parent));
    auto keypair = session_keys_->getParaKeyPair(validators);
    if (not keypair) {
      return std::nullopt;
    }
    OUTCOME_TRY(context, SigningContext::make(parachain_api_, relay_parent));
    return ValidatorSigner{
        keypair->second,
        context,
        std::move(keypair->first),
        hasher_,
        sr25519_provider_,
    };
  }

  outcome::result<std::optional<ValidatorIndex>>
  ValidatorSignerFactory::getAuthorityValidatorIndex(
      const primitives::BlockHash &relay_parent) {
    OUTCOME_TRY(session_index,
                parachain_api_->session_index_for_child(relay_parent));
    OUTCOME_TRY(session_info,
                parachain_api_->session_info(relay_parent, session_index));

    if (!session_info) {
      return std::nullopt;
    }

    auto keys = session_keys_->getAudiKeyPair(session_info->discovery_keys);
    if (keys != nullptr) {
      for (ValidatorIndex i = 0; i < session_info->discovery_keys.size(); ++i) {
        if (keys->public_key == session_info->discovery_keys[i]) {
          return i;
        }
      }
    }
    return std::nullopt;
  }
}  // namespace kagome::parachain
