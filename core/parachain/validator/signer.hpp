/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <optional>

#include "crypto/key_store/session_keys.hpp"
#include "crypto/sr25519_provider.hpp"
#include "network/types/collator_messages.hpp"
#include "network/types/collator_messages_vstaging.hpp"
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
    static auto toSignable(const crypto::Hasher &,
                           const network::vstaging::CompactStatement &v) {
      return v;
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

  class IValidatorSigner {
   public:
    virtual ~IValidatorSigner() = default;

    virtual outcome::result<IndexedAndSigned<network::Statement>> sign(
        const network::Statement &payload) const = 0;
    virtual outcome::result<IndexedAndSigned<scale::BitVec>> sign(
        const scale::BitVec &payload) const = 0;

    virtual ValidatorIndex validatorIndex() const = 0;
    virtual SessionIndex getSessionIndex() const = 0;
    virtual const primitives::BlockHash &relayParent() const = 0;
    virtual outcome::result<Signature> signRaw(
        common::BufferView data) const = 0;
  };

  /// Signs payload with signing context and validator keypair.
  class ValidatorSigner : public IValidatorSigner {
   public:
    using ValidatorIndex = network::ValidatorIndex;

    ValidatorSigner(ValidatorIndex validator_index,
                    SigningContext context,
                    std::shared_ptr<crypto::Sr25519Keypair> keypair,
                    std::shared_ptr<crypto::Hasher> hasher,
                    std::shared_ptr<crypto::Sr25519Provider> sr25519_provider);

    outcome::result<IndexedAndSigned<network::Statement>> sign(
        const network::Statement &payload) const override {
      return sign_obj(payload);
    }
    outcome::result<IndexedAndSigned<scale::BitVec>> sign(
        const scale::BitVec &payload) const override {
      return sign_obj(payload);
    }

    /// Sign payload.
    template <typename T>
    outcome::result<parachain::IndexedAndSigned<T>> sign_obj(T payload) const {
      auto data = context_.signable(*hasher_, payload);
      OUTCOME_TRY(signature, sr25519_provider_->sign(*keypair_, data));
      return parachain::IndexedAndSigned<T>{
          {std::move(payload), validator_index_},
          signature,
      };
    }

    outcome::result<Signature> signRaw(common::BufferView data) const override {
      return sr25519_provider_->sign(*keypair_, data);
    }

    SessionIndex getSessionIndex() const override;

    /// Get validator index.
    ValidatorIndex validatorIndex() const override;

    /// Get relay parent hash.
    const primitives::BlockHash &relayParent() const override;

   private:
    ValidatorIndex validator_index_;
    SigningContext context_;
    std::shared_ptr<crypto::Sr25519Keypair> keypair_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<crypto::Sr25519Provider> sr25519_provider_;
  };

  /// Creates validator signer.
  class IValidatorSignerFactory {
   public:
    virtual ~IValidatorSignerFactory() = default;

    /// Create validator signer if keypair belongs to validator at given block.
    virtual outcome::result<std::optional<std::shared_ptr<IValidatorSigner>>>
    at(const primitives::BlockHash &relay_parent) = 0;

    virtual outcome::result<std::optional<ValidatorIndex>>
    getAuthorityValidatorIndex(const primitives::BlockHash &relay_parent) = 0;
  };

  /// Creates validator signer.
  class ValidatorSignerFactory
      : public IValidatorSignerFactory,
        std::enable_shared_from_this<ValidatorSignerFactory> {
   public:
    ValidatorSignerFactory(
        std::shared_ptr<runtime::ParachainHost> parachain_api,
        std::shared_ptr<crypto::SessionKeys> session_keys,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<crypto::Sr25519Provider> sr25519_provider);

    /// Create validator signer if keypair belongs to validator at given block.
    outcome::result<std::optional<std::shared_ptr<IValidatorSigner>>> at(
        const primitives::BlockHash &relay_parent) override;

    outcome::result<std::optional<ValidatorIndex>> getAuthorityValidatorIndex(
        const primitives::BlockHash &relay_parent) override;

   private:
    std::shared_ptr<runtime::ParachainHost> parachain_api_;
    std::shared_ptr<crypto::SessionKeys> session_keys_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<crypto::Sr25519Provider> sr25519_provider_;
  };
}  // namespace kagome::parachain
