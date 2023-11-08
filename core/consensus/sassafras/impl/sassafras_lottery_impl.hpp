/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/sassafras/sassafras_lottery.hpp"

#include <memory>
#include <vector>

#include "consensus/sassafras/types/ticket.hpp"
#include "crypto/random_generator.hpp"
#include "log/logger.hpp"
#include "storage/buffer_map_types.hpp"

namespace kagome::application {
  class AppStateManager;
}

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
        application::AppStateManager &app_state_manager,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<crypto::CSPRNG> random_generator,
        std::shared_ptr<crypto::BandersnatchProvider> bandersnatch_provider,
        std::shared_ptr<crypto::Ed25519Provider> ed25519_provider,
        std::shared_ptr<crypto::VRFProvider> vrf_provider,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<runtime::SassafrasApi> api,
        std::shared_ptr<offchain::OffchainWorkerFactory> ocw_factory,
        std::shared_ptr<offchain::Runner> ocw_runner,
        std::shared_ptr<storage::SpacedStorage> storage);

    bool prepare();

    void changeEpoch(EpochNumber epoch,
                     const Randomness &randomness,
                     const Threshold &ticket_threshold,
                     const Threshold &threshold,
                     const crypto::BandersnatchKeypair &keypair,
                     AttemptsNumber attempts) override;

    EpochNumber getEpoch() const override;

    std::optional<crypto::VRFOutput> getSlotLeadership(
        const primitives::BlockHash &block,
        SlotNumber slot,
        bool allow_fallback) const override;

    crypto::VRFOutput slotVrfSignature(SlotNumber slot) const override;

    std::optional<primitives::AuthorityIndex> secondarySlotAuthor(
        SlotNumber slot,
        primitives::AuthorityListSize authorities_count,
        const Randomness &randomness) const override;

   private:
    void generateTickets();

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

    // Data of actual epoch
    EpochNumber epoch_;
    Randomness randomness_;
    std::vector<TicketId> ticket_ids_;
    std::vector<TicketEnvelope> tickets_;
    crypto::BandersnatchKeypair keypair_;

    // Data of next epoch
    EpochNumber next_epoch_;
    Randomness next_randomness_;
    std::optional<std::vector<TicketId>> next_ticket_ids_;
    std::optional<std::vector<TicketEnvelope>> next_tickets_;

    Threshold ticket_threshold_;
    Threshold threshold_;
    AttemptsNumber attempts_;
  };

}  // namespace kagome::consensus::sassafras
