/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 *
 * Kagome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * Kagome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KAGOME_BABE_LOTTERY_IMPL_HPP
#define KAGOME_BABE_LOTTERY_IMPL_HPP

#include <memory>
#include <vector>

#include "common/logger.hpp"
#include "consensus/babe/babe_lottery.hpp"
#include "crypto/hasher.hpp"
#include "crypto/vrf_provider.hpp"

namespace kagome::consensus {
  class BabeLotteryImpl : public BabeLottery {
   public:
    BabeLotteryImpl(std::shared_ptr<crypto::VRFProvider> vrf_provider,
                    std::shared_ptr<crypto::Hasher> hasher);

    SlotsLeadership slotsLeadership(
        const Epoch &epoch,
        const Threshold &threshold,
        const crypto::SR25519Keypair &keypair) const override;

    Randomness computeRandomness(const Randomness &last_epoch_randomness,
                                 EpochIndex last_epoch_index) override;

    void submitVRFValue(const crypto::VRFPreOutput &value) override;

   private:
    std::shared_ptr<crypto::VRFProvider> vrf_provider_;
    std::shared_ptr<crypto::Hasher> hasher_;

    /// also known as "rho" (greek letter) in the spec
    std::vector<crypto::VRFPreOutput> last_epoch_vrf_values_;
    common::Logger logger_;
  };
}  // namespace kagome::consensus

#endif  // KAGOME_BABE_LOTTERY_IMPL_HPP
