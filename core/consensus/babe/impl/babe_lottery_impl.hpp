/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/babe/babe_lottery.hpp"

#include <memory>
#include <vector>

#include "consensus/babe/types/babe_configuration.hpp"
#include "log/logger.hpp"

namespace kagome::consensus::babe {
  class BabeConfigRepository;
}

namespace kagome::crypto {
  class Hasher;
  class VRFProvider;
  class SessionKeys;
}  // namespace kagome::crypto

namespace kagome::consensus::babe {

  class BabeLotteryImpl : public BabeLottery {
   public:
    BabeLotteryImpl(std::shared_ptr<BabeConfigRepository> config_repo,
                    std::shared_ptr<crypto::SessionKeys> session_keys,
                    std::shared_ptr<crypto::VRFProvider> vrf_provider,
                    std::shared_ptr<crypto::Hasher> hasher);

    EpochNumber getEpoch() const override;

    bool changeEpoch(EpochNumber epoch,
                     const primitives::BlockInfo &best_block) override;

    std::optional<SlotLeadership> getSlotLeadership(
        const primitives::BlockHash &block, SlotNumber slot) const override;

   private:
    log::Logger logger_;

    std::shared_ptr<BabeConfigRepository> config_repo_;
    std::shared_ptr<crypto::SessionKeys> session_keys_;
    std::shared_ptr<crypto::VRFProvider> vrf_provider_;
    std::shared_ptr<crypto::Hasher> hasher_;

    using KeypairWithIndexOpt = std::optional<
        std::pair<std::shared_ptr<crypto::Sr25519Keypair>, AuthorityIndex>>;

    // Data of actual epoch
    EpochNumber epoch_;
    Randomness randomness_;
    AuthorityIndex auth_number_;
    KeypairWithIndexOpt keypair_;
    Threshold threshold_;
    AllowedSlots allowed_slots_;
  };
}  // namespace kagome::consensus::babe
