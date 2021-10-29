/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_VRF_PROVIDER_MOCK_HPP
#define KAGOME_VRF_PROVIDER_MOCK_HPP

#include <gmock/gmock.h>

#include "crypto/vrf_provider.hpp"

namespace kagome::crypto {
  struct VRFProviderMock : public VRFProvider {
    MOCK_CONST_METHOD0(generateKeypair, Sr25519Keypair());

    MOCK_CONST_METHOD3(sign,
                       std::optional<VRFOutput>(const common::Buffer &,
                                                const Sr25519Keypair &,
                                                const VRFThreshold &));
    MOCK_CONST_METHOD4(verify,
                       VRFVerifyOutput(const common::Buffer &,
                                       const VRFOutput &,
                                       const Sr25519PublicKey &,
                                       const VRFThreshold &));

    MOCK_CONST_METHOD3(signTranscript,
                       std::optional<VRFOutput>(const primitives::Transcript &,
                                                const Sr25519Keypair &,
                                                const VRFThreshold &));
    MOCK_CONST_METHOD4(verifyTranscript,
                       VRFVerifyOutput(const primitives::Transcript &,
                                       const VRFOutput &,
                                       const Sr25519PublicKey &,
                                       const VRFThreshold &));
  };
}  // namespace kagome::crypto

#endif  // KAGOME_VRF_PROVIDER_MOCK_HPP
