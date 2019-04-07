/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_LIBP2P_CRYPTO_ERROR_HPP
#define KAGOME_CORE_LIBP2P_CRYPTO_ERROR_HPP

#include <outcome/outcome.hpp>

namespace libp2p::crypto {
  enum class CryptoProviderError {
    kInvalidKeyType = 1,         ///< failed to unmarshal key type, wrong value
    kUnknownKeyType = 2,         ///< failed to unmarshal key
    kFailedToUnmarshalData = 3,  ///< protobuf error, failed to unmarshal data
  };
}

OUTCOME_HPP_DECLARE_ERROR(libp2p::crypto, CryptoProviderError)

#endif  // KAGOME_CORE_LIBP2P_CRYPTO_ERROR_HPP
