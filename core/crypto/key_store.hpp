/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <filesystem>
#include <memory>
#include <optional>

#include <libp2p/crypto/key.hpp>

#include "application/app_state_manager.hpp"
#include "common/bytestr.hpp"
#include "common/optref.hpp"
#include "crypto/bip39/bip39_provider.hpp"
#include "crypto/common.hpp"
#include "crypto/ecdsa_provider.hpp"
#include "crypto/ed25519_provider.hpp"
#include "crypto/ed25519_types.hpp"
#include "crypto/key_store/key_type.hpp"
#include "crypto/sr25519_provider.hpp"
#include "filesystem/common.hpp"
#include "log/logger.hpp"
#include "utils/json_unquote.hpp"
#include "utils/read_file.hpp"

namespace kagome::crypto {

  enum class KeyStoreError {
    UNSUPPORTED_KEY_TYPE = 1,
    UNSUPPORTED_CRYPTO_TYPE,
    WRONG_SEED_SIZE,
    KEY_NOT_FOUND,
    BABE_ALREADY_EXIST,
    GRAN_ALREADY_EXIST,
    AUDI_ALREADY_EXIST,
    WRONG_PUBLIC_KEY,
    FAILED_TO_OPEN_FILE,
    INVALID_FILE_FORMAT,
  };
}

OUTCOME_HPP_DECLARE_ERROR(kagome::crypto, KeyStoreError);

namespace kagome::crypto {
  template <typename T>
  concept Suite = requires(T t) {
    typename T::Keypair;
    typename T::PrivateKey;
    typename T::PublicKey;
    typename T::Seed;
  };

  template <Suite T>
  class KeySuiteStore {
   public:
    using Keypair = typename T::Keypair;
    using PrivateKey = typename T::PrivateKey;
    using PublicKey = typename T::PublicKey;
    using Seed = typename T::Seed;

    virtual ~KeySuiteStore() = default;

    /**
     * @brief generates a keypair and stores it in memory
     * @param key_type key type identifier
     * @param mnemonic_phrase mnemonic phrase
     * @return generated key pair or error
     */
    virtual outcome::result<Keypair> generateKeypair(
        KeyType key_type, std::string_view mnemonic_phrase) = 0;

    /**
     * @brief generates a keypair and stores it in memory
     * @param key_type key type identifier
     * @param seed seed for generating keys
     * @return generated key pair
     */
    virtual outcome::result<Keypair> generateKeypair(KeyType key_type,
                                                     const Seed &seed) = 0;

    /**
     * @brief generates a keypair and stores it on disk
     * @param key_type key type identifier
     * @return generated key pair or error
     */
    virtual outcome::result<Keypair> generateKeypairOnDisk(
        KeyType key_type) = 0;

    /**
     * @brief searches for key pair
     * @param key_type key category
     * @param pk public key to look for
     * @return found key pair if exists
     */
    virtual OptRef<const Keypair> findKeypair(KeyType key_type,
                                              const PublicKey &pk) const = 0;

    /**
     * @brief searches for public keys of specified type
     * @param key_type key type identifier to look for
     * @return vector of found public keys
     */
    virtual outcome::result<std::vector<PublicKey>> getPublicKeys(
        KeyType key_type) const = 0;
  };

  libp2p::crypto::KeyPair ed25519KeyToLibp2pKeypair(const Ed25519Keypair &kp);
  ;

  class KeyStore {
   public:
    struct Config {
      explicit Config(std::filesystem::path key_store_dir)
          : key_store_dir{std::move(key_store_dir)} {}

      std::filesystem::path key_store_dir;
    };

    KeyStore(std::unique_ptr<KeySuiteStore<Sr25519Provider>> sr25519,
             std::unique_ptr<KeySuiteStore<Ed25519Provider>> ed25519,
             std::unique_ptr<KeySuiteStore<EcdsaProvider>> ecdsa,
             std::shared_ptr<Ed25519Provider> ed25519_provider,
             std::shared_ptr<application::AppStateManager> app_manager,
             Config config);

    bool prepare();

    KeySuiteStore<Sr25519Provider> &sr25519() {
      return *sr25519_;
    }

    KeySuiteStore<Ed25519Provider> &ed25519() {
      return *ed25519_;
    }

    KeySuiteStore<EcdsaProvider> &ecdsa() {
      return *ecdsa_;
    }

    outcome::result<libp2p::crypto::KeyPair> loadLibp2pKeypair(
        const std::filesystem::path &file_path) const;

   private:
    using SecureString = std::
        basic_string<char, std::char_traits<char>, SecureHeapAllocator<char>>;

    outcome::result<SecureString> readSeed(
        const std::filesystem::path &file_path) const;

    outcome::result<void> scanKeyDirectory(const std::filesystem::path &dir);

    Config config_;
    std::unique_ptr<KeySuiteStore<Sr25519Provider>> sr25519_;
    std::unique_ptr<KeySuiteStore<Ed25519Provider>> ed25519_;
    std::unique_ptr<KeySuiteStore<EcdsaProvider>> ecdsa_;
    std::shared_ptr<Ed25519Provider> ed25519_provider_;
    std::shared_ptr<application::AppStateManager> app_manager_;
    log::Logger logger_;
  };

}  // namespace kagome::crypto
