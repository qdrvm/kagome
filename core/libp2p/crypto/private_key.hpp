/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PRIVATE_KEY_HPP
#define KAGOME_PRIVATE_KEY_HPP

#include "libp2p/crypto/key.hpp"

namespace libp2p::crypto {
  class PublicKey;
  /**
   * Represents private key
   */
  class PrivateKey : public Key {
   public:
    /**
     * @brief inherited constructor
     */
    using Key::Key;

    /**
     * @brief destructor
     */
    ~PrivateKey() override = default;
  };
}  // namespace libp2p::crypto

#endif  // KAGOME_PRIVATE_KEY_HPP
