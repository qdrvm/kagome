/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_KEY_HPP
#define KAGOME_KEY_HPP

#include <memory>
#include <string_view>
#include <utility>

#include "common/buffer.hpp"
#include "libp2p/crypto/crypto_common.hpp"
#include "libp2p/crypto/private_key.hpp"
#include "libp2p/crypto/public_key.hpp"

namespace libp2p::crypto {
  /**
   * Interface for keys and methods, which are related to them
   */
  class Key {
   public:
    /* INTERFACE */

    /**
     * Convert the key into Protobuf representation
     * @return bytes of Protobuf object
     */
    virtual kagome::common::Buffer marshal() const = 0;

    /**
     * Supported types of keys
     */
    enum class KeyType { kRSA, kED25519 };

    /**
     * Get type of this key
     * @return the type
     */
    virtual KeyType getType() const = 0;

    /**
     * Get a byte representation of the key
     * @return the bytes
     */
    virtual const kagome::common::Buffer &bytes() const = 0;

    virtual bool operator==(const Key &other) const = 0;

    /* INTERFACE ENDS */

    /**
     * Generate a key pair
     * @param key_type - desired type of the keys
     * @param bits - bitsize of the keys; minimum 1024
     * @return pair of <PubKey, PrivKey>
     */
    static std::pair<PublicKey, PrivateKey> generateKeyPair(KeyType key_type,
                                                            int bits);

    /**
     * Supported ECDH curves
     */
    enum class CurveType { kP256, kP284, kP521 };

    /**
     * Generate an ephemeral public key and return a function that will compute
     * the shared secret key
     * @param curve to be used in this ECDH
     * @return pair of EphemeralPublicKey and a function, which computes the
     * shared secret key
     */
    static std::pair<kagome::common::Buffer, std::function<PrivateKey()>>
    generateEphemeralKeyPair(CurveType curve);

    /**
     * Type of the stretched key
     */
    struct StretchedKey {
      kagome::common::Buffer iv;
      kagome::common::Buffer cipher_key;
      kagome::common::Buffer mac_key;
    };

    /**
     * Supported cipher types
     */
    enum class CipherType { kAES128, kAES256, kBlowfish };

    /**
     * Generate a set of keys for each party by stretching the shared key
     * @param cipher_type to be used
     * @param hash_type to be used
     * @param secret to be used
     * @return objects of type StretchedKey
     */
    static std::vector<StretchedKey> keyStretcher(
        CipherType cipher_type, common::HashType hash_type,
        const kagome::common::Buffer &secret);

    /**
     * Convert a protobuf representation into a private key
     * @param key - Protobuf bytes to be converted
     * @return private key
     */
    static PrivateKey unmarshalPrivateKey(const kagome::common::Buffer &key);

    /**
     * Convert a protobuf representation into a public key
     * @param key - Protobuf bytes to be converted
     * @return public key
     */
    static PublicKey unmarshalPublicKey(const kagome::common::Buffer &key);

    /**
     * Import a private key from a password-protected PEM file
     * @param pem_path - path to the .pem file
     * @param password of that file
     * @return private key from the file
     */
    static PrivateKey import(std::string_view pem_path,
                             std::string_view password);
  };
}  // namespace libp2p::crypto

#endif  // KAGOME_KEY_HPP
