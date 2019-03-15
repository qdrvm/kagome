/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_AES_HPP
#define KAGOME_AES_HPP

#include "common/buffer.hpp"

namespace libp2p::crypto {
  /**
   * Advanced Encryption Standard
   */
  class Aes {
   public:
    /**
     * Create an AES instance
     * @param key of this cipher; if 16 bytes are used, it is AES-128, for 32
     * bytes it is AES-256
     * @param iv - initial value; must have length 16
     */
    Aes(const kagome::common::Buffer &key, const kagome::common::Buffer &iv);

    /**
     * Encrypt the data using this instance's cipher
     * @param data to be encrypted
     * @return encrypted bytes
     */
    kagome::common::Buffer encrypt(const kagome::common::Buffer &data) const;

    /**
     * Decrypt the data using this instance's cipher
     * @param data to be decrypted
     * @return decrypted bytes
     */
    kagome::common::Buffer decrypt(const kagome::common::Buffer &data) const;
  };
}  // namespace libp2p::crypto

#endif  // KAGOME_AES_HPP
