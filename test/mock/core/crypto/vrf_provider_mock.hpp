/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_VRF_PROVIDER_MOCK_HPP
#define KAGOME_VRF_PROVIDER_MOCK_HPP

#include "crypto/vrf_provider.hpp"

#include <gmock/gmock.h>

namespace kagome::crypto {
  struct VRFProviderMock : public VRFProvider {
    MOCK_CONST_METHOD0(generateKeypair, SR25519Keypair());

    MOCK_CONST_METHOD3(sign,
                       boost::optional<VRFOutput>(const common::Buffer &,
                                                  const SR25519Keypair &,
                                                  const VRFValue &));

    MOCK_CONST_METHOD3(verify,
                       bool(const common::Buffer &,
                            const VRFOutput &,
                            const SR25519PublicKey &));
  };
}  // namespace kagome::crypto

#endif  // KAGOME_VRF_PROVIDER_MOCK_HPP
