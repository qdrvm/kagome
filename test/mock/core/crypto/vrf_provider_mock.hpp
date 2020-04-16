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

#ifndef KAGOME_VRF_PROVIDER_MOCK_HPP
#define KAGOME_VRF_PROVIDER_MOCK_HPP

#include <gmock/gmock.h>

#include "crypto/vrf_provider.hpp"

namespace kagome::crypto {
  struct VRFProviderMock : public VRFProvider {
    MOCK_CONST_METHOD0(generateKeypair, SR25519Keypair());

    MOCK_CONST_METHOD3(sign,
                       boost::optional<VRFOutput>(const common::Buffer &,
                                                  const SR25519Keypair &,
                                                  const VRFThreshold &));
    MOCK_CONST_METHOD4(verify,
                       VRFVerifyOutput(const common::Buffer &,
                            const VRFOutput &,
                            const SR25519PublicKey &,
                            const VRFThreshold &));
  };
}  // namespace kagome::crypto

#endif  // KAGOME_VRF_PROVIDER_MOCK_HPP
