/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_LIBP2P_CRYPTO_IMPL_DETAIL_AES_CRYPT_HPP
#define KAGOME_CORE_LIBP2P_CRYPTO_IMPL_DETAIL_AES_CRYPT_HPP

#include <outcome/outcome.hpp>
#include "common/buffer.hpp"
#include "libp2p/crypto/common.hpp"

namespace libp2p::crypto::aes {
  /**
   * @class AesCrypt provides methods for encryption and decryption by means of
   * AES128 and AES256
   */
  class AesProvider {
    using Buffer = kagome::common::Buffer;
    using Aes128Secret = libp2p::crypto::common::Aes128Secret;
    using Aes256Secret = libp2p::crypto::common::Aes256Secret;

   public:
    virtual ~AesProvider() = default;
    /**
     * @brief encrypts data using AES128 cipher
     * @param secret cipher secret
     * @param data plain data
     * @return encrypted data or error
     */
    virtual outcome::result<Buffer> encryptAesCtr128(
        const Aes128Secret &secret, const Buffer &data) const = 0;

    /**
     * @brief decrypts data using AES128 cipher
     * @param secret cipher secret
     * @param data encrypted data
     * @return decrypted data or error
     */
    virtual outcome::result<Buffer> decryptAesCtr128(
        const Aes128Secret &secret, const Buffer &data) const = 0;

    /**
     * @brief encrypts data using AES256 cipher
     * @param secret cipher secret
     * @param data plain data
     * @return encrypted data or error
     */
    virtual outcome::result<Buffer> encryptAesCtr256(
        const Aes256Secret &secret, const Buffer &data) const = 0;

    /**
     * @brief decrypts data using AES256 cipher
     * @param secret cipher secret
     * @param data encrypted data
     * @return decrypted data or error
     */
    virtual outcome::result<Buffer> decryptAesCtr256(
        const Aes256Secret &secret, const Buffer &data) const = 0;
  };
}  // namespace libp2p::crypto::aes

#endif  // KAGOME_CORE_LIBP2P_CRYPTO_IMPL_DETAIL_AES_CRYPT_HPP
