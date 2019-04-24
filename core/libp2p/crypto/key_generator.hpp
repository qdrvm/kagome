/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_LIBP2P_CRYPTO_KEY_GENERATOR_HPP
#define KAGOME_CORE_LIBP2P_CRYPTO_KEY_GENERATOR_HPP

#include "common/buffer.hpp"
#include "libp2p/crypto/common.hpp"

namespace libp2p::crypto {
  /**
   * @class KeyGenerator provides key generation functions
   */
  class KeyGenerator {
   public:
    virtual ~KeyGenerator() = default;

    /**
     * @brief generates key pair of specified type
     * @param key_type key type option
     * @return new generated key pair
     */
    virtual outcome::result<common::KeyPair> generateKeyPair(
        common::KeyType key_type) const = 0;

    /**
     * @brief derives public key from private key
     * @param private_key private key
     * @return derived public key
     */
    virtual outcome::result<PublicKey> derivePublicKey(
        const PrivateKey &private_key) const = 0;
    /**
     * Generate an ephemeral public key and return a function that will compute
     * the shared secret key
     * @param curve to be used in this ECDH
     * @return ephemeral key pair
     */
    virtual outcome::result<common::EphemeralKeyPair> generateEphemeralKeyPair(
        common::CurveType curve) const = 0;

    /**
     * Generate a set of keys for each party by stretching the shared key
     * @param cipher_type to be used
     * @param hash_type to be used
     * @param secret to be used
     * @return objects of type StretchedKey
     */
    virtual std::vector<common::StretchedKey> keyStretcher(
        common::CipherType cipher_type, common::HashType hash_type,
        const kagome::common::Buffer &secret) const = 0;
  };
}  // namespace libp2p::crypto

#endif  // KAGOME_CORE_LIBP2P_CRYPTO_KEY_GENERATOR_HPP
