/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/secp256k1/secp256k1_provider_impl.hpp"

namespace kagome::crypto::secp256k1 {
  EcdsaVerifyError convertFailureToError(outcome::result<void> failure) {
    if (!failure.has_error()) {
      return ecdsa_verify_error::kNoError;
    }
    if (failure == outcome::failure(Secp256k1ProviderError::INVALID_V_VALUE)) {
      return ecdsa_verify_error::kInvalidV;
    }
    if (failure
        == outcome::failure(Secp256k1ProviderError::INVALID_R_OR_S_VALUE)) {
      return ecdsa_verify_error::kInvalidRS;
    }

    return ecdsa_verify_error::kInvalidSignature;
  }
}  // namespace kagome::crypto::secp256k1
