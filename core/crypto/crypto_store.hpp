/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_STORE_HPP
#define KAGOME_CRYPTO_STORE_HPP

#include <memory>

#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <libp2p/crypto/key.hpp>

#include "crypto/bip39/bip39_provider.hpp"
#include "crypto/crypto_store/key_type.hpp"
#include "crypto/ed25519_types.hpp"
#include "crypto/secp256k1_types.hpp"
#include "crypto/sr25519_types.hpp"

namespace kagome::crypto {
  class CryptoStore {
   public:
    // currently std::filesystem::path is missing required methods in macos SDK
    // so we have to use boost's filesystem primitives
    using Path = boost::filesystem::path;

    virtual ~CryptoStore() = default;

    using Ed25519Keys = std::vector<Ed25519PublicKey>;
    using Sr25519Keys = std::vector<Sr25519PublicKey>;

    /**
     * @brief generates Ed25519 keypair and stores it in memory
     * @param key_type key type identifier
     * @param mnemonic_phrase mnemonic phrase
     * @return generated key pair or error
     */
    virtual outcome::result<Ed25519Keypair> generateEd25519Keypair(
        KeyTypeId key_type, std::string_view mnemonic_phrase) = 0;

    /**
     * @brief generates SR25519 keypair and stores it in memory
     * @param key_type key type identifier
     * @param mnemonic_phrase mnemonic phrase
     * @return generated key pair or error
     */
    virtual outcome::result<Sr25519Keypair> generateSr25519Keypair(
        KeyTypeId key_type, std::string_view mnemonic_phrase) = 0;

    /**
     * @brief generates Ed25519 keypair and stores it in memory
     * @param key_type key type identifier
     * @param seed seed for generating keys
     * @return generated key pair
     */
    virtual Ed25519Keypair generateEd25519Keypair(KeyTypeId key_type,
                                                  const Ed25519Seed &seed) = 0;

    /**
     * @brief generates SR25519 keypair and stores it in memory
     * @param key_type key type identifier
     * @param seed seed for generating keys
     * @return generated key
     */
    virtual Sr25519Keypair generateSr25519Keypair(KeyTypeId key_type,
                                                  const Sr25519Seed &seed) = 0;

    /**
     * @brief generates Ed25519 keypair and stores it on disk
     * @param key_type key type identifier
     * @return generated key pair or error
     */
    virtual outcome::result<Ed25519Keypair> generateEd25519KeypairOnDisk(
        KeyTypeId key_type) = 0;

    /**
     * @brief generates SR25519 keypair and stores it on disk
     * @param key_type key type identifier
     * @return generated key pair or error
     */
    virtual outcome::result<Sr25519Keypair> generateSr25519KeypairOnDisk(
        KeyTypeId key_type) = 0;

    /**
     * @brief searches for key pair
     * @param key_type key category
     * @param pk public key to look for
     * @return found key pair if exists
     */
    virtual outcome::result<Ed25519Keypair> findEd25519Keypair(
        KeyTypeId key_type, const Ed25519PublicKey &pk) const = 0;

    /**
     * @brief searches for key pair
     * @param key_type key category
     * @param pk public key to look for
     * @return found key pair if exists
     */
    virtual outcome::result<Sr25519Keypair> findSr25519Keypair(
        KeyTypeId key_type, const Sr25519PublicKey &pk) const = 0;

    /**
     * @brief searches for Ed25519 keys of specified type
     * @param key_type key type identifier to look for
     * @return vector of found public keys
     */
    virtual Ed25519Keys getEd25519PublicKeys(KeyTypeId key_type) const = 0;

    /**
     * @brief searches for SR25519 keys of specified typeED
     * @param key_type key type identifier to look for
     * @return vector of found public keys
     */
    virtual Sr25519Keys getSr25519PublicKeys(KeyTypeId key_type) const = 0;
  };
}  // namespace kagome::crypto

#endif  // KAGOME_CRYPTO_STORE_HPP
