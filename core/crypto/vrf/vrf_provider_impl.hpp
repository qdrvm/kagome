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

#ifndef KAGOME_CORE_CONSENSUS_VRF_VRF_HPP
#define KAGOME_CORE_CONSENSUS_VRF_VRF_HPP

#include "crypto/vrf_provider.hpp"

#include <boost/optional.hpp>
#include "common/buffer.hpp"
#include "crypto/random_generator.hpp"

namespace kagome::crypto {

  class VRFProviderImpl : public VRFProvider {
   public:
    explicit VRFProviderImpl(std::shared_ptr<CSPRNG> generator);

    ~VRFProviderImpl() override = default;

    SR25519Keypair generateKeypair() const override;

    boost::optional<VRFOutput> sign(const common::Buffer &msg,
                                    const SR25519Keypair &keypair,
                                    const VRFThreshold &threshold) const override;

    VRFVerifyOutput verify(const common::Buffer &msg,
                const VRFOutput &output,
                const SR25519PublicKey &public_key,
                const VRFThreshold &threshold) const override;

   private:
    std::shared_ptr<CSPRNG> generator_;
  };
}  // namespace kagome::crypto

#endif  // KAGOME_CORE_CONSENSUS_VRF_VRF_HPP
