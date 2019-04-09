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

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::crypto, OpenSslError, e) {
  using libp2p::crypto::OpenSslError;
  switch (e) {  // NOLINT
    case OpenSslError::kFailedInitializeContext:
      return "failed to initialize context";
    case OpenSslError::kFailedInitializeOperation:
      return "failed to initialize operation";
    case OpenSslError::kFailedEncryptUpdate:
      return "failed to update encryption";
    case OpenSslError::kFailedDecryptUpdate:
      return "failed to update decryption";
    case OpenSslError::kFailedEncryptFinalize:
      return "failed to finalize encryption";
    case OpenSslError::kFailedDecryptFinalize:
      return "failed to finalize decryption";
    case OpenSslError::kWrongIvSize:
      return "wrong iv size";
    default:
      break;
  }
  return "unknown CryptoProviderError";
}

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::crypto, MiscError, e) {
  using libp2p::crypto::MiscError;
  switch (e) {  // NOLINT
    case MiscError::kWrongArgumentValue:
      return "wrong argument value";
    default:
      break;
  }
  return "unknown MiscError";
}