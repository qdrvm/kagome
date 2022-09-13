/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "parachain/validator/signer.hpp"

namespace kagome::parachain {
  outcome::result<SigningContext> SigningContext::make(
      const std::shared_ptr<runtime::ParachainHost> &parachain_api,
      const primitives::BlockHash &relay_parent) {
    OUTCOME_TRY(session_index,
                parachain_api->session_index_for_child(relay_parent));
    return SigningContext{session_index, relay_parent};
  }

  ValidatorSigner::ValidatorSigner(
      ValidatorIndex validator_index,
      SigningContext context,
      std::shared_ptr<crypto::Sr25519Keypair> keypair,
      std::shared_ptr<crypto::Sr25519Provider> sr25519_provider)
      : validator_index_{validator_index},
        context_{context},
        keypair_{std::move(keypair)},
        sr25519_provider_{std::move(sr25519_provider)} {}

  ValidatorSigner::ValidatorIndex ValidatorSigner::validatorIndex() const {
    return validator_index_;
  }

  const SigningContext &ValidatorSigner::context() const {
    return context_;
  }

  ValidatorSignerFactory::ValidatorSignerFactory(
      std::shared_ptr<runtime::ParachainHost> parachain_api,
      std::shared_ptr<crypto::Sr25519Keypair> keypair,
      std::shared_ptr<crypto::Sr25519Provider> sr25519_provider)
      : parachain_api_{std::move(parachain_api)},
        keypair_{std::move(keypair)},
        sr25519_provider_{std::move(sr25519_provider)} {}

  outcome::result<std::optional<ValidatorSigner>> ValidatorSignerFactory::at(
      const primitives::BlockHash &relay_parent) {
    OUTCOME_TRY(validators, parachain_api_->validators(relay_parent));
    auto it =
        std::find(validators.begin(), validators.end(), keypair_->public_key);
    if (it == validators.end()) {
      return std::nullopt;
    }
    network::ValidatorIndex validator_index = it - validators.begin();
    OUTCOME_TRY(context, SigningContext::make(parachain_api_, relay_parent));
    return ValidatorSigner{
        validator_index,
        context,
        keypair_,
        sr25519_provider_,
    };
  }
}  // namespace kagome::parachain
