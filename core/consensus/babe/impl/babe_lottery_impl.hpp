/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BABE_LOTTERY_IMPL_HPP
#define KAGOME_BABE_LOTTERY_IMPL_HPP

#include <memory>
#include <vector>

#include "consensus/babe/babe_lottery.hpp"
#include "crypto/hasher.hpp"
#include "crypto/vrf_provider.hpp"
#include "log/logger.hpp"
#include "primitives/babe_configuration.hpp"

namespace kagome::consensus::babe {
  class BabeConfigRepository;
}

namespace kagome::consensus::babe {

  class BabeLotteryImpl : public BabeLottery {
   public:
    BabeLotteryImpl(std::shared_ptr<crypto::VRFProvider> vrf_provider,
                    std::shared_ptr<BabeConfigRepository> babe_config_repo,
                    std::shared_ptr<crypto::Hasher> hasher);

    void changeEpoch(const EpochDescriptor &epoch,
                     const Randomness &randomness,
                     const Threshold &threshold,
                     const crypto::Sr25519Keypair &keypair) override;

    EpochDescriptor getEpoch() const override;

    std::optional<crypto::VRFOutput> getSlotLeadership(
        primitives::BabeSlotNumber i) const override;

    crypto::VRFOutput slotVrfSignature(
        primitives::BabeSlotNumber slot) const override;

    std::optional<primitives::AuthorityIndex> secondarySlotAuthor(
        primitives::BabeSlotNumber slot,
        primitives::AuthorityListSize authorities_count,
        const Randomness &randomness) const override;

   private:
    std::shared_ptr<crypto::VRFProvider> vrf_provider_;
    std::shared_ptr<crypto::Hasher> hasher_;

    log::Logger logger_;

    EpochDescriptor epoch_;
    Randomness randomness_;
    Threshold threshold_;
    crypto::Sr25519Keypair keypair_;
  };
}  // namespace kagome::consensus::babe

#endif  // KAGOME_BABE_LOTTERY_IMPL_HPP
