/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/basic/scheduler.hpp>

#include "crypto/hasher.hpp"
#include "log/logger.hpp"
#include "parachain/availability/bitfield/store.hpp"
#include "parachain/availability/fetch/fetch.hpp"
#include "parachain/availability/store/store.hpp"
#include "parachain/validator/signer.hpp"
#include "primitives/event_types.hpp"
#include "runtime/runtime_api/parachain_host.hpp"

namespace kagome::parachain {
  class IBitfieldSigner {
   public:
    using BroadcastCallback = std::function<void(
        const primitives::BlockHash &, const network::SignedBitfield &)>;
    using Candidates = std::vector<std::optional<network::CandidateHash>>;

    virtual ~IBitfieldSigner() = default;
    virtual void start() = 0;

    /// Sign bitfield for given block.
    virtual outcome::result<void> sign(const IValidatorSigner &signer,
                                       const Candidates &candidates) = 0;

    virtual void setBroadcastCallback(BroadcastCallback &&callback) = 0;
  };

  /// Signs, stores and broadcasts bitfield for every new head.
  class BitfieldSigner : public IBitfieldSigner,
                         public std::enable_shared_from_this<BitfieldSigner> {
   public:
    BitfieldSigner(
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<ValidatorSignerFactory> signer_factory,
        std::shared_ptr<libp2p::basic::Scheduler> scheduler,
        std::shared_ptr<runtime::ParachainHost> parachain_api,
        std::shared_ptr<AvailabilityStore> store,
        std::shared_ptr<Fetch> fetch,
        std::shared_ptr<BitfieldStore> bitfield_store,
        primitives::events::ChainSubscriptionEnginePtr chain_sub_engine);

    /// Subscribes to new heads.
    void start() override;

    outcome::result<void> sign(const IValidatorSigner &signer,
                               const Candidates &candidates) override;

    void setBroadcastCallback(BroadcastCallback &&callback) override;

   private:
    using BlockHash = primitives::BlockHash;

    outcome::result<void> onBlock(const BlockHash &relay_parent);

    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<ValidatorSignerFactory> signer_factory_;
    std::shared_ptr<libp2p::basic::Scheduler> scheduler_;
    std::shared_ptr<runtime::ParachainHost> parachain_api_;
    std::shared_ptr<AvailabilityStore> store_;
    std::shared_ptr<Fetch> fetch_;
    std::shared_ptr<BitfieldStore> bitfield_store_;
    primitives::events::ChainSub chain_sub_;
    BroadcastCallback broadcast_;
    log::Logger logger_ = log::createLogger("BitfieldSigner", "parachain");
  };
}  // namespace kagome::parachain
