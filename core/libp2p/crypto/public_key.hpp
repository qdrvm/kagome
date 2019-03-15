/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PUBLIC_KEY_HPP
#define KAGOME_PUBLIC_KEY_HPP

#include "libp2p/crypto/key.hpp"

namespace libp2p::crypto {
  /**
   * Represents public key
   */
  class PublicKey : public Key {};
}  // namespace libp2p::crypto

#endif  // KAGOME_PUBLIC_KEY_HPP
