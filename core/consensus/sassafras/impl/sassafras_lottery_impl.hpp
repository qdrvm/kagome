/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/sassafras/sassafras_lottery.hpp"

#include <memory>
#include <vector>

#include "consensus/sassafras/types/sassafras_configuration.hpp"
#include "consensus/sassafras/types/ticket.hpp"
#include "crypto/random_generator.hpp"
#include "log/logger.hpp"
#include "storage/buffer_map_types.hpp"

namespace kagome::blockchain {
  class BlockTree;
}

namespace kagome::offchain {
  class Runner;
  class OffchainWorkerFactory;
}  // namespace kagome::offchain

namespace kagome::consensus::sassafras {
  class SassafrasConfigRepository;
}

namespace kagome::crypto {
  class Hasher;
  class VRFProvider;
  class BandersnatchProvider;
  class Ed25519Provider;
  class SessionKeys;
}  // namespace kagome::crypto

namespace kagome::runtime {
  class SassafrasApi;
}

namespace kagome::storage {
  class SpacedStorage;
}

namespace kagome::consensus::sassafras {

  class SassafrasLotteryImpl : public SassafrasLottery {
   public:
    SassafrasLotteryImpl(
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<crypto::CSPRNG> random_generator,
        std::shared_ptr<crypto::BandersnatchProvider> bandersnatch_provider,
        std::shared_ptr<crypto::Ed25519Provider> ed25519_provider,
        std::shared_ptr<crypto::VRFProvider> vrf_provider,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<runtime::SassafrasApi> api,
        std::shared_ptr<offchain::OffchainWorkerFactory> ocw_factory,
        std::shared_ptr<offchain::Runner> ocw_runner,
        std::shared_ptr<storage::SpacedStorage> storage,
        std::shared_ptr<SassafrasConfigRepository> sassafras_config_repo,
        std::shared_ptr<crypto::SessionKeys> session_keys);

    EpochNumber getEpoch() const override;

    bool changeEpoch(EpochNumber epoch,
                     const primitives::BlockInfo &best_block) override;

    std::optional<SlotLeadership> getSlotLeadership(
        const primitives::BlockHash &block, SlotNumber slot) const override;

   private:
    void load();
    void store() const;

    outcome::result<void> setupActualEpoch(
        EpochNumber epoch, const primitives::BlockInfo &best_block);
    outcome::result<void> generateTickets(
        EpochNumber epoch, const primitives::BlockInfo &best_block);

    SlotLeadership primarySlotLeadership(SlotNumber slot,
                                         const Ticket &ticket) const;
    SlotLeadership secondarySlotLeadership(SlotNumber slot) const;

    log::Logger logger_;

    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<crypto::CSPRNG> random_generator_;
    std::shared_ptr<crypto::BandersnatchProvider> bandersnatch_provider_;
    std::shared_ptr<crypto::Ed25519Provider> ed25519_provider_;
    std::shared_ptr<crypto::VRFProvider> vrf_provider_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<runtime::SassafrasApi> api_;
    std::shared_ptr<offchain::OffchainWorkerFactory> ocw_factory_;
    std::shared_ptr<offchain::Runner> ocw_runner_;
    std::shared_ptr<storage::BufferStorage> storage_;
    std::shared_ptr<SassafrasConfigRepository> sassafras_config_repo_;
    std::shared_ptr<crypto::SessionKeys> session_keys_;

    using KeypairWithIndexOpt =
        std::optional<std::pair<std::shared_ptr<crypto::BandersnatchKeypair>,
                                primitives::AuthorityIndex>>;

    // Data of actual epoch
    EpochNumber epoch_;
    Randomness randomness_;
    AuthorityIndex auth_number_;
    // AuthorityIndex auth_index_;
    KeypairWithIndexOpt keypair_;
    Tickets tickets_;

    // Data of next epoch
    std::optional<Tickets> next_tickets_;
  };

}  // namespace kagome::consensus::sassafras
