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
#include <outcome/outcome.hpp>

#include "common/buffer.hpp"
#include "libp2p/crypto/common.hpp"
#include "libp2p/crypto/private_key.hpp"
#include "libp2p/crypto/public_key.hpp"

namespace libp2p::crypto {
  /**
   * Contains cryptographic features, needed for Libp2p functioning
   */
  class CryptoProvider {
   protected:
    using Buffer = kagome::common::Buffer;

   public:
    virtual ~CryptoProvider() = default;
    /// AES features

    /**
     * Encrypt the data using AES-128
     * @param secret - key and initial value, needed for encryption
     * @param data to be encrypted
     * @return encrypted bytes, if encryption is successful, error otherwise
     */
    virtual outcome::result<Buffer> aesEncrypt(
        const common::Aes128Secret &secret, const Buffer &data) const = 0;

    /**
     * Encrypt the data using AES-256
     * @param secret - key and initial value, needed for encryption
     * @param data to be encrypted
     * @return encrypted bytes, if encryption is successful, error otherwise
     */
    virtual outcome::result<Buffer> aesEncrypt(
        const common::Aes256Secret &secret, const Buffer &data) const = 0;

    /**
     * Decrypt the data using AES-128
     * @param secret - key and initial value, needed for decryption
     * @param data to be decrypted
     * @return decrypted bytes, if decryption is successful, error otherwise
     */
    virtual outcome::result<Buffer> aesDecrypt(
        const common::Aes128Secret &secret, const Buffer &data) const = 0;

    /**
     * Decrypt the data using AES-256
     * @param secret - key and initial value, needed for decryption
     * @param data to be decrypted
     * @return decrypted bytes, if decryption is successful, error otherwise
     */
    virtual outcome::result<Buffer> aesDecrypt(
        const common::Aes256Secret &secret, const Buffer &data) const = 0;

    /// HMAC features

    /**
     * Hash the data
     * @param hash to be used
     * @param secret to be used
     * @param data to be hashed
     * @return hashed bytes if hash was successful, error otherwise
     */
    virtual outcome::result<Buffer> hmacDigest(common::HashType hash,
                                               const Buffer &secret,
                                               const Buffer &data) = 0;

    /// keys features

    /**
     * Generate a ED25519 key pair
     * @return key pair
     */
    virtual common::KeyPair generateEd25519Keypair() const = 0;

    /**
     * Generate a RSA key pair
     * @param key_type - desired type of the key
     * @return key pair
     */
    virtual common::KeyPair generateRSAKeypair(
        common::RSAKeyType key_type) const = 0;

    /**
     * Generate an ephemeral public key and return a function that will compute
     * the shared secret key
     * @param curve to be used in this ECDH
     * @return ephemeral key pair
     */
    virtual common::EphemeralKeyPair generateEphemeralKeyPair(
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
        const Buffer &secret) const = 0;

    /**
     * Convert the public key into Protobuf representation
     * @param key - public key to be mashalled
     * @return bytes of Protobuf object if marshalling was successful, error
     * otherwise
     */
    virtual outcome::result<Buffer> marshal(const PublicKey &key) const = 0;

    /**
     * Convert the private key into Protobuf representation
     * @param key - public key to be mashalled
     * @return bytes of Protobuf object if marshalling was successful, error
     * otherwise
     */
    virtual outcome::result<Buffer> marshal(const PrivateKey &key) const = 0;

    /**
     * Convert Protobuf representation of public key into the object
     * @param key_bytes - bytes of the public key
     * @return public key in case of success, error otherwise
     */
    virtual outcome::result<PublicKey> unmarshalPublicKey(
        const Buffer &key_bytes) const = 0;

    /**
     * Convert Protobuf representation of private key into the object
     * @param key_bytes - bytes of the private key
     * @return private key in case of success, error otherwise
     */
    virtual outcome::result<PrivateKey> unmarshalPrivateKey(
        const Buffer &key_bytes) const = 0;

    /**
     * Import a private key from a password-protected PEM file
     * @param pem_path - path to the .pem file
     * @param password of that file
     * @return private key from the file if import was successful, error
     * otherwise
     */
    virtual outcome::result<PrivateKey> import(
        boost::filesystem::path pem_path, std::string_view password) const = 0;

    /// misc utilities

    /**
     * Get a secure hash of the password
     * @param password to be hashed
     * @param salt to be used in hashing process
     * @param iterations - number of iterations
     * @param key_size - size of the key in bytes
     * @param hash - hashing algorithm
     * @return a new password
     */
    virtual Buffer pbkdf2(std::string_view password, const Buffer &salt,
                          uint64_t iterations, size_t key_size,
                          common::HashType hash) const = 0;

    /**
     * @brief derives public key from private key
     * @param private_key private key
     * @return derived public key or error if private key is invalid or another
     * error occured
     */
    virtual outcome::result<PublicKey> derivePublicKey(
        const PrivateKey &private_key) const = 0;
  };
}  // namespace libp2p::crypto

#endif  // KAGOME_CRYPTO_PROVIDER_HPP
