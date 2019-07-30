
/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CRYPTO_VRF_TYPES
#define KAGOME_CORE_CRYPTO_VRF_TYPES

#include <boost/multiprecision/cpp_int.hpp>
#include "common/blob.hpp"
extern "C" {
#include <sr25519/sr25519.h>
};

namespace kagome::crypto {

  using VRFValue = boost::multiprecision::uint256_t;

  using VRFProof = std::array<uint8_t, SR25519_VRF_PROOF_SIZE>;

  struct VRFOutput {
    VRFValue value;
    VRFProof proof{};
  };

  using VRFSecretKey = common::Blob<SR25519_SECRET_SIZE>;

  using VRFPublicKey = common::Blob<SR25519_PUBLIC_SIZE>;

  struct VRFKeypair {
    VRFSecretKey secret_key;
    VRFPublicKey public_key;
  };

}  // namespace kagome::crypto

#endif  // KAGOME_CORE_CRYPTO_VRF_TYPES
