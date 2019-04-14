/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/crypto/error.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::crypto, CryptoProviderError, e) {
  using libp2p::crypto::CryptoProviderError;
  switch (e) {  // NOLINT
    case CryptoProviderError::INVALID_KEY_TYPE:
      return "failed to unmarshal key type, invalid value";
    case CryptoProviderError::UNKNOWN_KEY_TYPE:
      return "failed to unmarshal key";
    case CryptoProviderError::FAILED_UNMARSHAL_DATA:
      return "google protobuf error, failed to unmarshal data";
    default:
      break;
  }
  return "unknown CryptoProviderError code";
}

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::crypto, OpenSslError, e) {
  using libp2p::crypto::OpenSslError;
  switch (e) {  // NOLINT
    case OpenSslError::FAILED_INITIALIZE_CONTEXT:
      return "failed to initialize context";
    case OpenSslError::FAILED_INITIALIZE_OPERATION:
      return "failed to initialize operation";
    case OpenSslError::FAILED_ENCRYPT_UPDATE:
      return "failed to update encryption";
    case OpenSslError::FAILED_DECRYPT_UPDATE:
      return "failed to update decryption";
    case OpenSslError::FAILED_ENCRYPT_FINALIZE:
      return "failed to finalize encryption";
    case OpenSslError::FAILED_DECRYPT_FINALIZE:
      return "failed to finalize decryption";
    case OpenSslError::WRONG_IV_SIZE:
      return "wrong iv size";
    default:
      break;
  }
  return "unknown CryptoProviderError code";
}

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::crypto, MiscError, e) {
  using libp2p::crypto::MiscError;
  switch (e) {  // NOLINT
    case MiscError::WRONG_ARGUMENT_VALUE:
      return "wrong argument value";
    default:
      break;
  }
  return "unknown MiscError code";
}

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::crypto, HmacProviderError, e) {
  using libp2p::crypto::HmacProviderError;
  switch (e) {  // NOLINT
    case HmacProviderError::UNSUPPORTED_HASH_METHOD:
      return "hash method id provided is not supported";
    case HmacProviderError::FAILED_CREATE_CONTEXT:
      return "failed to create context";
    case HmacProviderError::FAILED_INITIALIZE_CONTEXT:
      return "failed to initialize context";
    case HmacProviderError::FAILED_UPDATE_DIGEST:
      return "failed to update digest";
    case HmacProviderError::FAILED_FINALIZE_DIGEST:
      return "failed to finalize digest";
    case HmacProviderError::WRONG_DIGEST_SIZE:
      return "wrong digest size";
    default:
      break;
  }
  return "unknown HmacProviderError code";
}

OUTCOME_CPP_DEFINE_CATEGORY(libp2p::crypto, RandomProviderError, e) {
  using libp2p::crypto::RandomProviderError;
  switch (e) {  // NOLINT
    case RandomProviderError::FAILED_OPEN_FILE:
      return "failed to open file at specified path";
    case RandomProviderError::TOKEN_NOT_EXISTS:
      return "specified random source doesn't exist in system";
    case RandomProviderError::FAILED_FETCH_BYTES:
      return "failed to fetch bytes from source";
    case RandomProviderError::INVALID_PROVIDER_TYPE:
      return "invalid or unsupported random provider type";
    default:
      break;
  }
  return "unknown RandomProviderError code";
}
