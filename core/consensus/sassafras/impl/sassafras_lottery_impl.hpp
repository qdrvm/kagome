/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/sassafras/sassafras_lottery.hpp"

#include <memory>
#include <vector>

#include "consensus/babe/types/babe_configuration.hpp"
#include "consensus/sassafras/types/ticket.hpp"
#include "crypto/random_generator.hpp"
#include "log/logger.hpp"

namespace kagome::blockchain {
  class BlockTree;
}

namespace kagome::consensus::sassafras {
  class SassafrasConfigRepository;
}

namespace kagome::crypto {
  class Hasher;
  class VRFProvider;
  class BandersnatchProvider;
}  // namespace kagome::crypto

namespace kagome::runtime {
  class SassafrasApi;
}

namespace kagome::consensus::sassafras {

  class SassafrasLotteryImpl : public SassafrasLottery {
   public:
    SassafrasLotteryImpl(
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<crypto::CSPRNG> random_generator,
        std::shared_ptr<crypto::BandersnatchProvider> bandersnatch_provider,
        std::shared_ptr<crypto::VRFProvider> vrf_provider,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<runtime::SassafrasApi> api);

    void changeEpoch(EpochNumber epoch,
                     const Randomness &randomness,
                     const Threshold &ticket_threshold,
                     const Threshold &threshold,
                     const crypto::BandersnatchKeypair &keypair) override;

    EpochNumber getEpoch() const override;

    std::optional<crypto::VRFOutput> getSlotLeadership(
        const primitives::BlockHash &block, SlotNumber i) const override;

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
    std::shared_ptr<crypto::VRFProvider> vrf_provider_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<runtime::SassafrasApi> api_;

    EpochNumber epoch_;
    Randomness randomness_;
    EpochNumber next_epoch_;
    Randomness next_randomness_;
    Threshold ticket_threshold_;
    Threshold threshold_;
    crypto::BandersnatchKeypair keypair_;

    struct {
      std::vector<std::tuple<uint32_t, TicketId>> tickets;
      EpochNumber epoch_for;
      uint32_t next_send_index;
    } tickets_;
  };

}  // namespace kagome::consensus::sassafras
