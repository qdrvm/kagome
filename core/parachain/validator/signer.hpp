/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PARACHAIN_VALIDATOR_SIGNER_HPP
#define KAGOME_PARACHAIN_VALIDATOR_SIGNER_HPP

#include <optional>

#include "crypto/crypto_store/session_keys.hpp"
#include "crypto/sr25519_provider.hpp"
#include "network/types/collator_messages.hpp"
#include "runtime/runtime_api/parachain_host.hpp"

namespace kagome::parachain {
  /// A type returned by runtime with current session index and a parent hash.
  class SigningContext {
   private:
    static auto &toSignable(const crypto::Hasher &, const scale::BitVec &v) {
      return v;
    }
    static auto toSignable(const crypto::Hasher &hasher,
                           const network::CandidateState &v) {
      constexpr std::array<uint8_t, 4> kMagic{'B', 'K', 'N', 'G'};
      return std::make_tuple(
          kMagic, static_cast<uint8_t>(v.which()), candidateHash(hasher, v));
    }
    static auto toSignable(const crypto::Hasher &hasher,
                           const network::Statement &v) {
      return toSignable(hasher, v.candidate_state);
    }

   public:
    SCALE_TIE(2);

    /// Make signing context for given block.
    static outcome::result<SigningContext> make(
        const std::shared_ptr<runtime::ParachainHost> &parachain_api,
        const primitives::BlockHash &relay_parent);

    /// Make signable message for payload.
    template <typename T>
    auto signable(const crypto::Hasher &hasher, const T &payload) const {
      auto &&signable = toSignable(hasher, payload);
      return scale::encode(std::tie(signable, *this)).value();
    }

    /// Current session index.
    runtime::SessionIndex session_index;
    /// Hash of the parent.
    primitives::BlockHash relay_parent;
  };

  /// Signs payload with signing context and validator keypair.
  class ValidatorSigner {
   public:
    using ValidatorIndex = network::ValidatorIndex;

    ValidatorSigner(ValidatorIndex validator_index,
                    SigningContext context,
                    std::shared_ptr<crypto::Sr25519Keypair> keypair,
                    std::shared_ptr<crypto::Hasher> hasher,
                    std::shared_ptr<crypto::Sr25519Provider> sr25519_provider);

    /// Sign payload.
    template <typename T>
    outcome::result<parachain::IndexedAndSigned<T>> sign(T payload) const {
      auto data = context_.signable(*hasher_, payload);
      OUTCOME_TRY(signature, sr25519_provider_->sign(*keypair_, data));
      return parachain::IndexedAndSigned<T>{
          std::move(payload),
          validator_index_,
          signature,
      };
    }

    SessionIndex getSessionIndex() const;

    /// Get validator index.
    ValidatorIndex validatorIndex() const;

    /// Get relay parent hash.
    const primitives::BlockHash &relayParent() const;

   private:
    ValidatorIndex validator_index_;
    SigningContext context_;
    std::shared_ptr<crypto::Sr25519Keypair> keypair_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<crypto::Sr25519Provider> sr25519_provider_;
  };

  /// Creates validator signer.
  class ValidatorSignerFactory {
   public:
    ValidatorSignerFactory(
        std::shared_ptr<runtime::ParachainHost> parachain_api,
        std::shared_ptr<crypto::SessionKeys> session_keys,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<crypto::Sr25519Provider> sr25519_provider);

    /// Create validator signer if keypair belongs to validator at given block.
    outcome::result<std::optional<ValidatorSigner>> at(
        const primitives::BlockHash &relay_parent);

   private:
    std::shared_ptr<runtime::ParachainHost> parachain_api_;
    std::shared_ptr<crypto::SessionKeys> session_keys_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<crypto::Sr25519Provider> sr25519_provider_;
  };
}  // namespace kagome::parachain

#endif  // KAGOME_PARACHAIN_VALIDATOR_SIGNER_HPP
