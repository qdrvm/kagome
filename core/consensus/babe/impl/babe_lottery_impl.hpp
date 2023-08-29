/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "consensus/babe/babe_lottery.hpp"

#include <memory>
#include <vector>

#include "log/logger.hpp"
#include "primitives/babe_configuration.hpp"

namespace kagome::consensus::babe {
  class BabeConfigRepository;
}

namespace kagome::crypto {
  class Hasher;
  class VRFProvider;
}  // namespace kagome::crypto

namespace kagome::consensus::babe {

  class BabeLotteryImpl : public BabeLottery {
   public:
    BabeLotteryImpl(std::shared_ptr<crypto::VRFProvider> vrf_provider,
                    std::shared_ptr<BabeConfigRepository> babe_config_repo,
                    std::shared_ptr<crypto::Hasher> hasher);

    void changeEpoch(EpochNumber epoch,
                     const Randomness &randomness,
                     const Threshold &threshold,
                     const crypto::Sr25519Keypair &keypair) override;

    EpochNumber getEpoch() const override;

    std::optional<crypto::VRFOutput> getSlotLeadership(
        SlotNumber i) const override;

    crypto::VRFOutput slotVrfSignature(SlotNumber slot) const override;

    std::optional<primitives::AuthorityIndex> secondarySlotAuthor(
        SlotNumber slot,
        primitives::AuthorityListSize authorities_count,
        const Randomness &randomness) const override;

   private:
    std::shared_ptr<crypto::VRFProvider> vrf_provider_;
    std::shared_ptr<crypto::Hasher> hasher_;

    log::Logger logger_;

    EpochNumber epoch_;
    Randomness randomness_;
    Threshold threshold_;
    crypto::Sr25519Keypair keypair_;
  };
}  // namespace kagome::consensus::babe
