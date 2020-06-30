/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_STORE_HPP
#define KAGOME_CRYPTO_STORE_HPP

#include <filesystem>
#include <memory>

#include <boost/optional.hpp>
#include <libp2p/crypto/key.hpp>
#include "crypto/bip39/bip39_provider.hpp"
#include "crypto/ed25519_types.hpp"
#include "crypto/key_type.hpp"
#include "crypto/secp256k1_types.hpp"
#include "crypto/sr25519_types.hpp"

namespace kagome::crypto {
  class CryptoStore {
   public:
    virtual ~CryptoStore() = default;

    using ED25519Keys = std::vector<ED25519PublicKey>;
    using SR25519Keys = std::vector<SR25519PublicKey>;

    /**
     * @brief initializes key storage: loads keys from disk
     * @param keys_directory path to keys directory
     * @return success if loaded successfully or failure otherwise
     */
    virtual outcome::result<void> initialize(
        std::filesystem::path keys_directory) = 0;

    /**
     * @brief generates ED25519 keypair and stores it in memory
     * @param key_type key type identifier
     * @param mnemonic_phrase mnemonic phrase
     * @return generated key pair or error
     */
    virtual outcome::result<ED25519Keypair> generateEd25519Keypair(
        KeyTypeId key_type, std::string_view mnemonic_phrase) = 0;

    /**
     * @brief generates SR25519 keypair and stores it in memory
     * @param key_type key type identifier
     * @param mnemonic_phrase mnemonic phrase
     * @return generated key pair or error
     */
    virtual outcome::result<SR25519Keypair> generateSr25519Keypair(
        KeyTypeId key_type, std::string_view mnemonic_phrase) = 0;

    /**
     * @brief generates ED25519 keypair and stores it in memory
     * @param key_type key type identifier
     * @param seed seed for generating keys
     * @return generated key pair or error
     */
    virtual outcome::result<ED25519Keypair> generateEd25519Keypair(
        KeyTypeId key_type, const ED25519Seed &seed) = 0;

    /**
     * @brief generates SR25519 keypair and stores it in memory
     * @param key_type key type identifier
     * @param seed seed for generating keys
     * @return generated key pair or error
     */
    virtual outcome::result<SR25519Keypair> generateSr25519Keypair(
        KeyTypeId key_type, const SR25519Seed &seed) = 0;

    /**
     * @brief generates ED25519 keypair and stores it on disk
     * @param key_type key type identifier
     * @return generated key pair or error
     */
    virtual outcome::result<ED25519Keypair> generateEd25519KeyPair(
        KeyTypeId key_type) = 0;

    /**
     * @brief generates SR25519 keypair and stores it on disk
     * @param key_type key type identifier
     * @return generated key pair or error
     */
    virtual outcome::result<SR25519Keypair> generateSr25519KeyPair(
        KeyTypeId key_type) = 0;

    /**
     * @brief searches for key pair
     * @param key_type key category
     * @param pk public key to look for
     * @return found key pair if exists
     */
    virtual boost::optional<ED25519Keypair> findEd25519Keypair(
        KeyTypeId key_type, const ED25519PublicKey &pk) const = 0;

    /**
     * @brief searches for key pair
     * @param key_type key category
     * @param pk public key to look for
     * @return found key pair if exists
     */
    virtual boost::optional<SR25519Keypair> findSr25519Keypair(
        KeyTypeId key_type, const SR25519PublicKey &pk) const = 0;

    /**
     * @brief searches for ED25519 keys of specified type
     * @param key_type key type identifier to look for
     * @return vector of found public keys
     */
    virtual ED25519Keys getEd25519PublicKeys(KeyTypeId key_type) const = 0;

    /**
     * @brief searches for SR25519 keys of specified typeED
     * @param key_type key type identifier to look for
     * @return vector of found public keys
     */
    virtual SR25519Keys getSr25519PublicKeys(KeyTypeId key_type) const = 0;
  };
}  // namespace kagome::crypto

#endif  // KAGOME_CRYPTO_STORE_HPP
