/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gmock/gmock.h>

#include "crypto/vrf_provider.hpp"

namespace kagome::crypto {
  struct VRFProviderMock : public VRFProvider {
    MOCK_METHOD(Sr25519Keypair, generateKeypair, (), (const, override));

    MOCK_METHOD(std::optional<VRFOutput>,
                sign,
                (const common::Buffer &,
                 const Sr25519Keypair &,
                 const VRFThreshold &),
                (const, override));

    MOCK_METHOD(VRFVerifyOutput,
                verify,
                (const common::Buffer &,
                 const VRFOutput &,
                 const Sr25519PublicKey &,
                 const VRFThreshold &),
                (const, override));

    MOCK_METHOD(std::optional<VRFOutput>,
                signTranscript,
                (const primitives::Transcript &,
                 const Sr25519Keypair &,
                 const VRFThreshold &),
                (const, override));

    MOCK_METHOD(std::optional<VRFOutput>,
                signTranscript,
                (const primitives::Transcript &, const Sr25519Keypair &),
                (const, override));

    MOCK_METHOD(VRFVerifyOutput,
                verifyTranscript,
                (const primitives::Transcript &,
                 const VRFOutput &,
                 const Sr25519PublicKey &,
                 const VRFThreshold &),
                (const, override));
  };
}  // namespace kagome::crypto
