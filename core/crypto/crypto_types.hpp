/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_CRYPTO_VRF_TYPES
#define KAGOME_CORE_CRYPTO_VRF_TYPES

#include <boost/multiprecision/cpp_int.hpp>
#include <gsl/span>
#include "common/blob.hpp"
#include <sr25519/sr25519.h>

namespace kagome::crypto {

  using VRFValue = boost::multiprecision::uint256_t;

  using VRFProof = std::array<uint8_t, SR25519_VRF_PROOF_SIZE>;

  struct VRFOutput {
    VRFValue value;
    VRFProof proof{};
  };

  using SR25519SecretKey = common::Blob<SR25519_SECRET_SIZE>;

  using SR25519PublicKey = common::Blob<SR25519_PUBLIC_SIZE>;

  struct SR25519Keypair {
    SR25519SecretKey secret_key;
    SR25519PublicKey public_key;

    SR25519Keypair() = default;

    explicit SR25519Keypair(gsl::span<uint8_t, SR25519_KEYPAIR_SIZE> kp);
  };

}  // namespace kagome::crypto

#endif  // KAGOME_CORE_CRYPTO_VRF_TYPES
