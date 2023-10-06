/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PARACHAIN_AVAILABILITY_BITFIELD_SIGNER_HPP
#define KAGOME_PARACHAIN_AVAILABILITY_BITFIELD_SIGNER_HPP

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
  /// Signs, stores and broadcasts bitfield for every new head.
  class BitfieldSigner : public std::enable_shared_from_this<BitfieldSigner> {
   public:
    using BroadcastCallback = std::function<void(
        const primitives::BlockHash &, const network::SignedBitfield &)>;
    using Candidates = std::vector<std::optional<network::CandidateHash>>;

    BitfieldSigner(std::shared_ptr<crypto::Hasher> hasher,
                   std::shared_ptr<ValidatorSignerFactory> signer_factory,
                   std::shared_ptr<libp2p::basic::Scheduler> scheduler,
                   std::shared_ptr<runtime::ParachainHost> parachain_api,
                   std::shared_ptr<AvailabilityStore> store,
                   std::shared_ptr<Fetch> fetch,
                   std::shared_ptr<BitfieldStore> bitfield_store);

    /// Subscribes to new heads.
    void start(std::shared_ptr<primitives::events::ChainSubscriptionEngine>
                   chain_sub_engine);

    /// Sign bitfield for given block.
    outcome::result<void> sign(const ValidatorSigner &signer,
                               const Candidates &candidates);

    void setBroadcastCallback(BroadcastCallback &&callback);

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
    std::shared_ptr<primitives::events::ChainEventSubscriber> chain_sub_;
    BroadcastCallback broadcast_;
    log::Logger logger_ = log::createLogger("BitfieldSigner", "parachain");
  };
}  // namespace kagome::parachain

#endif  // KAGOME_PARACHAIN_AVAILABILITY_BITFIELD_SIGNER_HPP
