/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_PROVIDER_HPP
#define KAGOME_CRYPTO_PROVIDER_HPP

#include <memory>
#include <string_view>
#include <utility>

#include <boost/filesystem.hpp>
#include "common/buffer.hpp"
#include "libp2p/crypto/crypto_common.hpp"
#include "libp2p/crypto/private_key.hpp"
#include "libp2p/crypto/public_key.hpp"

namespace libp2p::crypto {
  /**
   * Contains cryptographic features, needed for Libp2p functioning
   */
  class CryptoProvider {
   public:
    /// AES features

    /**
     * Encrypt the data using AES-128
     * @param secret - key and initial value, needed for encryption
     * @param data to be encrypted
     * @return encrypted bytes
     */
    virtual kagome::common::Buffer aesEncrypt(
        const common::Aes128Secret &secret,
        const kagome::common::Buffer &data) const = 0;

    /**
     * Encrypt the data using AES-256
     * @param secret - key and initial value, needed for encryption
     * @param data to be encrypted
     * @return encrypted bytes
     */
    virtual kagome::common::Buffer aesEncrypt(
        const common::Aes256Secret &secret,
        const kagome::common::Buffer &data) const = 0;

    /**
     * Decrypt the data using AES-128
     * @param secret - key and initial value, needed for decryption
     * @param data to be decrypted
     * @return decrypted bytes
     */
    virtual kagome::common::Buffer aesDecrypt(
        const common::Aes128Secret &secret,
        const kagome::common::Buffer &data) const = 0;

    /**
     * Decrypt the data using AES-256
     * @param secret - key and initial value, needed for decryption
     * @param data to be decrypted
     * @return decrypted bytes
     */
    virtual kagome::common::Buffer aesDecrypt(
        const common::Aes256Secret &secret,
        const kagome::common::Buffer &data) const = 0;

    /// HMAC features

    /**
     * Hash the data
     * @param hash to be used
     * @param secret to be used
     * @param data to be hashed
     * @return hashed bytes
     */
    virtual kagome::common::Buffer hmacDigest(
        common::HashType hash, const kagome::common::Buffer &secret,
        const kagome::common::Buffer &data) = 0;

    /// keys features

    /**
     * Generate a key pair
     * @param key_type - desired type of the keys
     * @param bits - bitsize of the keys; minimum 1024
     * @return pair of <PubKey, PrivKey>
     */
    virtual std::pair<PublicKey, PrivateKey> generateKeyPair(
        common::KeyType key_type, uint32_t bits) const = 0;

    /**
     * Generate an ephemeral public key and return a function that will compute
     * the shared secret key
     * @param curve to be used in this ECDH
     * @return pair of EphemeralPublicKey and a function, which computes the
     * shared secret key
     */
    virtual std::pair<kagome::common::Buffer, std::function<PrivateKey()>>
    generateEphemeralKeyPair(common::CurveType curve) const = 0;

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

    /**
     * Convert the public key into Protobuf representation
     * @param key - public key to be mashalled
     * @return bytes of Protobuf object
     */
    virtual kagome::common::Buffer marshal(const PublicKey &key) const = 0;

    /**
     * Convert the private key into Protobuf representation
     * @param key - public key to be mashalled
     * @return bytes of Protobuf object
     */
    virtual kagome::common::Buffer marshal(const PrivateKey &key) const = 0;

    /**
     * Convert Protobuf representation of public key into the object
     * @param key_bytes - bytes of the public key
     * @return public key
     */
    virtual PublicKey unmarshalPublicKey(
        const kagome::common::Buffer &key_bytes) const = 0;

    /**
     * Convert Protobuf representation of private key into the object
     * @param key_bytes - bytes of the private key
     * @return private key
     */
    virtual PrivateKey unmarshalPrivateKey(
        const kagome::common::Buffer &key_bytes) const = 0;

    /**
     * Import a private key from a password-protected PEM file
     * @param pem_path - path to the .pem file
     * @param password of that file
     * @return private key from the file
     */
    virtual PrivateKey import(boost::filesystem::path pem_path,
                              std::string_view password) const = 0;

    /// misc utilities

    /**
     * Generate a Buffer with specified length populated by random bytes
     * @param number - size of the random buffer
     * @return random bytes
     */
    virtual kagome::common::Buffer randomBytes(size_t number) const = 0;

    /**
     * Get a secure hash of the password
     * @param password to be hashed
     * @param salt to be used in hashing process
     * @param iterations - number of iterations
     * @param key_size - size of the key in bytes
     * @param hash - hashing algorithm
     * @return a new password
     */
    virtual std::string pbkdf2(std::string_view password, std::string_view salt,
                               uint64_t iterations, size_t key_size,
                               common::HashType hash) const = 0;
  };
}  // namespace libp2p::crypto

#endif  // KAGOME_CRYPTO_PROVIDER_HPP
