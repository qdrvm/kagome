/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_STORE_HPP
#define KAGOME_CRYPTO_STORE_HPP

#include <memory>
#include <optional>

#include <libp2p/crypto/key.hpp>

#include "crypto/bip39/bip39_provider.hpp"
#include "crypto/crypto_store/key_type.hpp"
#include "crypto/ecdsa_types.hpp"
#include "crypto/ed25519_types.hpp"
#include "crypto/secp256k1_types.hpp"
#include "crypto/sr25519_types.hpp"
#include "filesystem/common.hpp"

namespace kagome::crypto {
  class CryptoStore {
   public:
    using Path = filesystem::path;

    virtual ~CryptoStore() = default;

    using EcdsaKeys = std::vector<EcdsaPublicKey>;
    using Ed25519Keys = std::vector<Ed25519PublicKey>;
    using Sr25519Keys = std::vector<Sr25519PublicKey>;
    using EcdsaKeypairs = std::vector<EcdsaKeypair>;
    using Ed25519Keypairs = std::vector<Ed25519Keypair>;
    using Sr25519Keypairs = std::vector<Sr25519Keypair>;

    /**
     * @brief generates ecdsa keypair and stores it in memory
     * @param key_type key type identifier
     * @param mnemonic_phrase mnemonic phrase
     * @return generated key pair or error
     */
    virtual outcome::result<EcdsaKeypair> generateEcdsaKeypair(
        KeyTypeId key_type, std::string_view mnemonic_phrase) = 0;

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
     * @brief generates ecdsa keypair and stores it in memory
     * @param key_type key type identifier
     * @param seed seed for generating keys
     * @return generated key pair
     */
    virtual outcome::result<EcdsaKeypair> generateEcdsaKeypair(
        KeyTypeId key_type, const EcdsaSeed &seed) = 0;

    /**
     * @brief generates Ed25519 keypair and stores it in memory
     * @param key_type key type identifier
     * @param seed seed for generating keys
     * @return generated key pair
     */
    virtual outcome::result<Ed25519Keypair> generateEd25519Keypair(
        KeyTypeId key_type, const Ed25519Seed &seed) = 0;

    /**
     * @brief generates SR25519 keypair and stores it in memory
     * @param key_type key type identifier
     * @param seed seed for generating keys
     * @return generated key
     */
    virtual outcome::result<Sr25519Keypair> generateSr25519Keypair(
        KeyTypeId key_type, const Sr25519Seed &seed) = 0;

    /**
     * @brief generates ecdsa keypair and stores it on disk
     * @param key_type key type identifier
     * @return generated key pair or error
     */
    virtual outcome::result<EcdsaKeypair> generateEcdsaKeypairOnDisk(
        KeyTypeId key_type) = 0;

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
    virtual outcome::result<EcdsaKeypair> findEcdsaKeypair(
        KeyTypeId key_type, const EcdsaPublicKey &pk) const = 0;

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
     * @brief searches for ecdsa keys of specified type
     * @param key_type key type identifier to look for
     * @return vector of found public keys
     */
    virtual outcome::result<EcdsaKeys> getEcdsaPublicKeys(
        KeyTypeId key_type) const = 0;

    /**
     * @brief searches for Ed25519 keys of specified type
     * @param key_type key type identifier to look for
     * @return vector of found public keys
     */
    virtual outcome::result<Ed25519Keys> getEd25519PublicKeys(
        KeyTypeId key_type) const = 0;

    /**
     * @brief searches for SR25519 keys of specified type
     * @param key_type key type identifier to look for
     * @return vector of found public keys
     */
    virtual outcome::result<Sr25519Keys> getSr25519PublicKeys(
        KeyTypeId key_type) const = 0;

    /**
     * @return current LibP2P keypair
     */
    virtual std::optional<libp2p::crypto::KeyPair> getLibp2pKeypair() const = 0;

    /**
     * Acquires the key from user-provided path or generates and saves the
     * key under the path. Used when --node-key-file flag gets processed.
     * @param path - path the key file (raw bytes or hex-encoded)
     * @return LibP2P keypair
     */
    virtual outcome::result<libp2p::crypto::KeyPair> loadLibp2pKeypair(
        const Path &key_path) const = 0;
  };
}  // namespace kagome::crypto

#endif  // KAGOME_CRYPTO_STORE_HPP
