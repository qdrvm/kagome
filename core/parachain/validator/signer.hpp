/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PARACHAIN_VALIDATOR_SIGNER_HPP
#define KAGOME_PARACHAIN_VALIDATOR_SIGNER_HPP

#include <optional>

#include "crypto/sr25519_provider.hpp"
#include "network/types/collator_messages.hpp"
#include "runtime/runtime_api/parachain_host.hpp"

namespace kagome::parachain {
  /// A type returned by runtime with current session index and a parent hash.
  struct SigningContext {
    SCALE_TIE(2);

    static outcome::result<SigningContext> make(
        const std::shared_ptr<runtime::ParachainHost> &parachain_api,
        const primitives::BlockHash &relay_parent);

    template <typename T>
    auto signable(const T &payload) const {
      return scale::encode(std::tie(payload, *this));
    }

    /// Current session index.
    runtime::SessionIndex session_index;
    /// Hash of the parent.
    primitives::BlockHash relay_parent;
  };

  class ValidatorSigner {
   public:
    using ValidatorIndex = network::ValidatorIndex;

    ValidatorSigner(ValidatorIndex validator_index,
                    SigningContext context,
                    std::shared_ptr<crypto::Sr25519Keypair> keypair,
                    std::shared_ptr<crypto::Sr25519Provider> sr25519_provider);

    template <typename T>
    outcome::result<network::Signed<T>> sign(T payload) const {
      OUTCOME_TRY(data, context_.signable(payload));
      OUTCOME_TRY(signature, sr25519_provider_->sign(*keypair_, data));
      return network::Signed<T>{
          std::move(payload),
          validator_index_,
          signature,
      };
    }

    ValidatorIndex validatorIndex() const;
    const SigningContext &context() const;

   private:
    ValidatorIndex validator_index_;
    SigningContext context_;
    std::shared_ptr<crypto::Sr25519Keypair> keypair_;
    std::shared_ptr<crypto::Sr25519Provider> sr25519_provider_;
  };

  class ValidatorSignerFactory {
   public:
    ValidatorSignerFactory(
        std::shared_ptr<runtime::ParachainHost> parachain_api,
        std::shared_ptr<crypto::Sr25519Keypair> keypair,
        std::shared_ptr<crypto::Sr25519Provider> sr25519_provider);

    outcome::result<std::optional<ValidatorSigner>> at(
        const primitives::BlockHash &relay_parent);

   private:
    std::shared_ptr<runtime::ParachainHost> parachain_api_;
    std::shared_ptr<crypto::Sr25519Keypair> keypair_;
    std::shared_ptr<crypto::Sr25519Provider> sr25519_provider_;
  };
}  // namespace kagome::parachain

#endif  // KAGOME_PARACHAIN_VALIDATOR_SIGNER_HPP
