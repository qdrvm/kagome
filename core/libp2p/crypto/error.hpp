/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_LIBP2P_CRYPTO_ERROR_HPP
#define KAGOME_CORE_LIBP2P_CRYPTO_ERROR_HPP

#include <outcome/outcome.hpp>

namespace libp2p::crypto {
  enum class CryptoProviderError {
    kInvalidKeyType = 1,     ///< failed to unmarshal key type, wrong value
    kUnknownKeyType,         ///< failed to unmarshal key
    kFailedToUnmarshalData,  ///< protobuf error, failed to unmarshal data
  };

  enum class OpenSslError {
    kFailedInitializeContext = 1,  ///< failed to initialize context
    kFailedInitializeOperation,    ///< failed to initialize operation
    kFailedEncryptUpdate,          ///< failed to update encryption
    kFailedDecryptUpdate,          ///< failed to update decryption
    kFailedEncryptFinalize,        ///< failed to finalize encryption
    kFailedDecryptFinalize,        ///< failed to finalize decryption
    kWrongIvSize,                  ///< wrong iv size
  };

  enum class MiscError {
    kWrongArgumentValue = 1,  ///< wrong argument value
  };
}  // namespace libp2p::crypto

OUTCOME_HPP_DECLARE_ERROR(libp2p::crypto, CryptoProviderError)
OUTCOME_HPP_DECLARE_ERROR(libp2p::crypto, OpenSslError)
OUTCOME_HPP_DECLARE_ERROR(libp2p::crypto, MiscError)

#endif  // KAGOME_CORE_LIBP2P_CRYPTO_ERROR_HPP
