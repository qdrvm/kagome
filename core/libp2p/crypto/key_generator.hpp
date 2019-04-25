/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_LIBP2P_CRYPTO_KEY_GENERATOR_HPP
#define KAGOME_CORE_LIBP2P_CRYPTO_KEY_GENERATOR_HPP

#include <boost/filesystem.hpp>
#include "common/buffer.hpp"
#include "libp2p/crypto/common.hpp"

namespace libp2p::crypto {
  /**
   * @class KeyGenerator provides interface for key generation functions
   */
  class KeyGenerator {
   public:
    virtual ~KeyGenerator() = default;
    /**
     * @brief generates RSA key pair
     * @param key_type RSA key type means number of bits option
     * @return new generated key pair of public and private key or error
     */
    virtual outcome::result<common::KeyPair> generateRsa(
        common::RSAKeyType key_type) const = 0;

    /**
     * @brief generates ED25519 key pair
     * @return new generated key pair of public and private key or error
     */
    virtual outcome::result<common::KeyPair> generateEd25519() const = 0;

    /**
     * @brief generates SECP256k1 key pair
     * @return new generated key pair of public and private key or error
     */
    virtual outcome::result<common::KeyPair> generateSecp256k1() const = 0;

    /**
     * @brief derives public key from private key
     * @param private_key private key
     * @return derived public key or error
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
    virtual std::vector<common::StretchedKey> stretchKey(
        common::CipherType cipher_type, common::HashType hash_type,
        const kagome::common::Buffer &secret) const = 0;

    /**
     * Import a private key from a password-protected PEM file
     * @param pem_path - path to the .pem file
     * @param password of that file
     * @return private key from the file
     */
    virtual outcome::result<PrivateKey> import(
        boost::filesystem::path pem_path, std::string_view password) const = 0;
  };
}  // namespace libp2p::crypto

#endif  // KAGOME_CORE_LIBP2P_CRYPTO_KEY_GENERATOR_HPP
