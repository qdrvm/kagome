/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/production_consensus.hpp"

#include "clock/clock.hpp"
#include "consensus/sassafras/phase.hpp"
#include "consensus/sassafras/types/sassafras_configuration.hpp"
#include "consensus/sassafras/vrf.hpp"
#include "injector/lazy.hpp"
#include "log/logger.hpp"
#include "metrics/metrics.hpp"
#include "primitives/block.hpp"
#include "telemetry/service.hpp"

namespace kagome::consensus::sassafras {
  class SassafrasConfigRepository;  // FIXME STAB
}

namespace kagome::application {
  class AppConfiguration;
}

namespace kagome::blockchain {
  class BlockTree;
  //  class DigestTracker;
}  // namespace kagome::blockchain

namespace kagome::consensus {
  class SlotsUtil;
}

namespace kagome::consensus::sassafras {
  class SassafrasConfigRepository;
  class SassafrasLottery;
}  // namespace kagome::consensus::sassafras

namespace kagome::crypto {
  class SessionKeys;
}

namespace kagome::dispute {
  class DisputeCoordinator;
}

namespace kagome::parachain {
  class BitfieldStore;
  class BackingStore;
}  // namespace kagome::parachain

namespace kagome::consensus::sassafras {

  class Sassafras final : public ProductionConsensus,
                          public std::enable_shared_from_this<Sassafras> {
   public:
    struct Context {
      primitives::BlockInfo parent;
      EpochNumber epoch;
      SlotNumber slot;
      TimePoint slot_timestamp;
      std::shared_ptr<crypto::BandersnatchKeypair> keypair;
    };

    Sassafras(const application::AppConfiguration &app_config,
              const clock::SystemClock &clock,
              std::shared_ptr<blockchain::BlockTree> block_tree,
              LazySPtr<SlotsUtil> slots_util,
              std::shared_ptr<SassafrasConfigRepository> sassafras_config_repo,
              std::shared_ptr<crypto::SessionKeys> session_keys,
              std::shared_ptr<SassafrasLottery> lottery,
              std::shared_ptr<parachain::BitfieldStore> bitfield_store,
              std::shared_ptr<parachain::BackingStore> backing_store,
              std::shared_ptr<dispute::DisputeCoordinator> dispute_coordinator);

    ValidatorStatus getValidatorStatus(const primitives::BlockInfo &parent_info,
                                       EpochNumber epoch_number) const override;

    outcome::result<SlotNumber> getSlot(
        const primitives::BlockHeader &header) const override;

    outcome::result<void> processSlot(
        SlotNumber slot, const primitives::BlockInfo &best_block) override;

   private:
    outcome::result<primitives::PreRuntime> calculatePreDigest(
        const Context &ctx,
        std::optional<std::reference_wrapper<const crypto::VRFOutput>> output,
        primitives::AuthorityIndex authority_index) const;

    outcome::result<primitives::Seal> sealBlock(
        const Context &ctx, const primitives::Block &block) const;

    outcome::result<void> processSlotLeadership(
        const Context &ctx,
        TimePoint slot_timestamp,
        std::optional<std::reference_wrapper<const crypto::VRFOutput>> output,
        primitives::AuthorityIndex authority_index);

    void changeLotteryEpoch(const Context &ctx,
                            const EpochNumber &epoch,
                            primitives::AuthorityIndex authority_index,
                            const Epoch &sassafras_config) const;

    log::Logger log_;

    const clock::SystemClock &clock_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    LazySPtr<SlotsUtil> slots_util_;
    std::shared_ptr<SassafrasConfigRepository> sassafras_config_repo_;
    std::shared_ptr<crypto::SessionKeys> session_keys_;
    std::shared_ptr<SassafrasLottery> lottery_;
    std::shared_ptr<parachain::BitfieldStore> bitfield_store_;
    std::shared_ptr<parachain::BackingStore> backing_store_;
    std::shared_ptr<dispute::DisputeCoordinator> dispute_coordinator_;

    const bool is_validator_by_config_;

    // Metrics
    metrics::RegistryPtr metrics_registry_ = metrics::createRegistry();
    metrics::Gauge *metric_is_relaychain_validator_;

    telemetry::Telemetry telemetry_;  // telemetry
  };

}  // namespace kagome::consensus::sassafras
