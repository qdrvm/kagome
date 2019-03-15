/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_COMMON_HPP
#define KAGOME_CRYPTO_COMMON_HPP

namespace libp2p::crypto::common {
  /**
   * Supported hash types
   */
  enum class HashType { kSHA1, kSHA256, kSHA512 };
}  // namespace libp2p::crypto::common

#endif  // KAGOME_CRYPTO_COMMON_HPP
