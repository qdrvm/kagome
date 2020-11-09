/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_STORE_IMPL_HPP
#define KAGOME_CRYPTO_STORE_IMPL_HPP

#include <boost/filesystem.hpp>
#include <boost/variant.hpp>

#include "common/blob.hpp"
#include "common/logger.hpp"
#include "crypto/bip39/bip39_provider.hpp"
#include "crypto/bip39/mnemonic.hpp"
#include "crypto/crypto_store.hpp"
#include "crypto/ed25519_provider.hpp"
#include "crypto/random_generator.hpp"
#include "crypto/secp256k1_provider.hpp"
#include "crypto/sr25519_provider.hpp"

namespace kagome::crypto {

  namespace store {
    using KeyPair = boost::variant<Ed25519Keypair, Sr25519Keypair>;
    using PublicKey = common::Blob<32>;
    using Seed = common::Hash256;

    enum class CryptoId { ED25519, SR25519, SECP256k1 };
  }  // namespace store

  enum class CryptoStoreError {
    WRONG_KEYFILE_NAME = 1,
    UNSUPPORTED_KEY_TYPE,
    UNSUPPORTED_CRYPTO_TYPE,
    NOT_REGULAR_FILE,
    FAILED_OPEN_FILE,
    FILE_DOESNT_EXIST,
    INVALID_FILE_FORMAT,
    INCONSISTENT_KEYFILE,
    WRONG_SEED_SIZE,
    KEY_NOT_FOUND,
    KEYS_PATH_IS_NOT_DIRECTORY,
    FAILED_CREATE_KEYS_DIRECTORY
  };

  class CryptoStoreImpl : public CryptoStore {
   public:
    // currently std::filesystem::path is missing required methods in macos SDK

    ~CryptoStoreImpl() override = default;

    CryptoStoreImpl(std::shared_ptr<Ed25519Provider> ed25519_provider,
                    std::shared_ptr<Sr25519Provider> sr25519_provider,
                    std::shared_ptr<Secp256k1Provider> secp256k1_provider,
                    std::shared_ptr<Bip39Provider> bip39_provider,
                    std::shared_ptr<CSPRNG> random_generator);

    /**
     * @brief ensures that keys directory exists
     * @param keys_directory keys directory
     * @return success if exists or managed to create, failure otherwise
     */
    outcome::result<void> initialize(Path keys_directory);

    outcome::result<Ed25519Keypair> generateEd25519Keypair(
        KeyTypeId key_type, std::string_view mnemonic_phrase) override;

    outcome::result<Sr25519Keypair> generateSr25519Keypair(
        KeyTypeId key_type, std::string_view mnemonic_phrase) override;

    Ed25519Keypair generateEd25519Keypair(KeyTypeId key_type,
                                          const Ed25519Seed &seed) override;

    Sr25519Keypair generateSr25519Keypair(KeyTypeId key_type,
                                          const Sr25519Seed &seed) override;

    outcome::result<Ed25519Keypair> generateEd25519KeypairOnDisk(
        KeyTypeId key_type) override;

    outcome::result<Sr25519Keypair> generateSr25519KeypairOnDisk(
        KeyTypeId key_type) override;

    outcome::result<Ed25519Keypair> findEd25519Keypair(
        KeyTypeId key_type, const Ed25519PublicKey &pk) const override;

    outcome::result<Sr25519Keypair> findSr25519Keypair(
        KeyTypeId key_type, const Sr25519PublicKey &pk) const override;

    Ed25519Keys getEd25519PublicKeys(KeyTypeId key_type) const override;

    Sr25519Keys getSr25519PublicKeys(KeyTypeId key_type) const override;

   private:
    outcome::result<std::pair<KeyTypeId, store::PublicKey>> parseKeyFileName(
        std::string_view file_name) const;

    Path composeKeyPath(KeyTypeId key_type,
                        const store::PublicKey &public_key) const;

    outcome::result<void> storeKeyfile(KeyTypeId key_type,
                                       const store::PublicKey &public_key,
                                       const store::Seed &seed);

    static outcome::result<std::string> loadFileContent(
        const boost::filesystem::path &file_path);

    template <typename KeypairType,
              typename SeedType,
              typename Provider,
              typename PublicKeyType,
              typename SecretKeyType>
    outcome::result<KeypairType> generateKeypair(
        KeyTypeId key_type,
        std::string_view mnemonic_phrase,
        std::shared_ptr<Provider> provider,
        std::map<KeyTypeId, std::map<PublicKeyType, SecretKeyType>> &keys) {
      OUTCOME_TRY(bip_seed, bip39_provider_->generateSeed(mnemonic_phrase));
      if (bip_seed.size() < SeedType::size()) {
        return CryptoStoreError::WRONG_SEED_SIZE;
      }
      auto seed_span = gsl::make_span(bip_seed.data(), SeedType::size());
      OUTCOME_TRY(seed, SeedType::fromSpan(seed_span));
      auto pair = provider->generateKeypair(seed);
      keys[key_type].emplace(pair.public_key, pair.secret_key);
      return pair;
    }

    void loadKeys(
        KeyTypeId key_type,
        const std::function<void(std::string_view,
                                 std::string_view,
                                 store::PublicKey const &)> &on_loaded) const;
    template <typename SeedType,
              typename PublicKeyType,
              typename SecretKeyType,
              typename Provider>
    std::vector<PublicKeyType> getPublicKeys(
        KeyTypeId key_type,
        const std::map<KeyTypeId, std::map<PublicKeyType, SecretKeyType>>
            &keys_cache,
        std::shared_ptr<Provider> provider) const {
      namespace fs = boost::filesystem;
      std::set<store::PublicKey> found_keys;
      // iterate over in-memory map
      if (keys_cache.find(key_type) != keys_cache.end()) {
        const auto &map = keys_cache.at(key_type);
        for (const auto &k : map) {
          found_keys.emplace(k.first);
        }
      }
      bool keys_dir_exists =
          fs::exists(keys_directory_) and fs::is_directory(keys_directory_);
      if (not keys_dir_exists) {
        logger_->error("Failed to open crypto storage directory: {}",
                       keys_directory_.string());
        return {};
      }
      loadKeys(key_type,
               [this, &found_keys, &provider](std::string_view filename,
                                              std::string_view content,
                                              store::PublicKey const &pk) {
                 auto &&seed = SeedType::fromHex(content);
                 if (!seed) {
                   logger_->error("failed to load seed from keyfile {} : {}",
                                  filename,
                                  seed.error().message());
                   return;
                 }
                 auto pair = provider->generateKeypair(seed.value());
                 if (pair.public_key == pk) {
                   found_keys.emplace(pk);
                 }
               });
      return std::vector<PublicKeyType>(found_keys.begin(), found_keys.end());
    }

    Path keys_directory_;
    std::shared_ptr<Ed25519Provider> ed25519_provider_;
    std::shared_ptr<Sr25519Provider> sr25519_provider_;
    std::shared_ptr<Secp256k1Provider> secp256k1_provider_;
    std::shared_ptr<Bip39Provider> bip39_provider_;
    std::shared_ptr<CSPRNG> random_generator_;

    std::map<KeyTypeId, std::map<Ed25519PublicKey, Ed25519PrivateKey>> ed_keys_;
    std::map<KeyTypeId, std::map<Sr25519PublicKey, Sr25519SecretKey>> sr_keys_;
    common::Logger logger_;
  };
}  // namespace kagome::crypto

OUTCOME_HPP_DECLARE_ERROR(kagome::crypto, CryptoStoreError);

#endif  // KAGOME_CRYPTO_STORE_IMPL_HPP
