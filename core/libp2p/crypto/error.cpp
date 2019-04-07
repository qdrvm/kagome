/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/crypto/error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::crypto, CryptoProviderError, e) {
  using libp2p::crypto::CryptoProviderError;
  switch (e) {  // NOLINT
    case CryptoProviderError::kInvalidKeyType:
      return "failed to unmarshal key type, invalid value";
    case CryptoProviderError::kUnknownKeyType:
      return "failed to unmarshal key";
    case CryptoProviderError::kFailedToUnmarshalData:
      return "google protobuf error, failed to unmarshal data";
    default:
      break;
  }
  return "unknown CryptoProviderError";
}
