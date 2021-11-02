/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CRYPTO_STORE_IMPL_HPP
#define KAGOME_CRYPTO_STORE_IMPL_HPP

#include <unordered_set>

#include <boost/filesystem.hpp>
#include <boost/variant.hpp>

#include "common/blob.hpp"
#include "crypto/bip39/bip39_provider.hpp"
#include "crypto/bip39/mnemonic.hpp"
#include "crypto/crypto_store.hpp"
#include "crypto/crypto_store/crypto_suites.hpp"
#include "crypto/crypto_store/key_cache.hpp"
#include "crypto/crypto_store/key_file_storage.hpp"
#include "log/logger.hpp"

namespace kagome::crypto {

  enum class CryptoStoreError {
    UNSUPPORTED_KEY_TYPE = 1,
    UNSUPPORTED_CRYPTO_TYPE,
    WRONG_SEED_SIZE,
    KEY_NOT_FOUND,
    BABE_ALREADY_EXIST,
    GRAN_ALREADY_EXIST
  };

  /// TODO(Harrm) Add policies to emit a warning when found a keypair
  /// with incompatible type and algorithm (e. g. ed25519 BABE keypair,
  /// whereas BABE has to be sr25519 only) or when trying to generate more
  /// keypair than there should be (e. g. more than one libp2p keypair is a
  /// suspicious behaviour)
  class CryptoStoreImpl : public CryptoStore {
   public:
    CryptoStoreImpl(std::shared_ptr<Ed25519Suite> ed_suite,
                    std::shared_ptr<Sr25519Suite> sr_suite,
                    std::shared_ptr<Bip39Provider> bip39_provider,
                    std::shared_ptr<KeyFileStorage> key_fs);

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

    outcome::result<Ed25519Keys> getEd25519PublicKeys(
        KeyTypeId key_type) const override;

    outcome::result<Sr25519Keys> getSr25519PublicKeys(
        KeyTypeId key_type) const override;

    std::optional<libp2p::crypto::KeyPair> getLibp2pKeypair() const override;

    outcome::result<libp2p::crypto::KeyPair> loadLibp2pKeypair(
        const Path &key_path) const override;

   private:
    template <typename CryptoSuite>
    outcome::result<std::vector<typename CryptoSuite::PublicKey>> getPublicKeys(
        KeyTypeId key_type,
        const KeyCache<CryptoSuite> &cache,
        const CryptoSuite &suite) const {
      auto cached_keys = cache.getPublicKeys();
      OUTCOME_TRY(keys, file_storage_->collectPublicKeys(key_type));

      std::vector<typename CryptoSuite::PublicKey> res;
      res.reserve(keys.size());
      for (auto &key : keys) {
        OUTCOME_TRY(pk, suite.toPublicKey(key));
        auto erased = cached_keys.erase(pk);
        // if we erased pk from cache, it means it was there and thus was a
        // valid cached key, which we can collect to our result
        if (erased == 1) {
          res.emplace_back(std::move(pk));

          // otherwise, pk was not found in cache and has to be loaded and
          // checked
        } else {
          // need to check if the read key's algorithm belongs to the given
          // CryptoSuite
          OUTCOME_TRY(seed_bytes, file_storage_->searchForSeed(key_type, key));
          BOOST_ASSERT_MSG(
              seed_bytes,
              "The public key has just been scanned, its file has to exist");
          if (not seed_bytes) {
            logger_->error("Error reading key seed from key file storage");
            continue;
          }
          auto seed_res = suite.toSeed(seed_bytes.value());
          if (not seed_res) {
            // cannot create a seed from file content; suppose it belongs to a
            // different algorithm
            continue;
          }
          SL_TRACE(logger_, "Loaded key {}", pk.toHex());
          auto kp = suite.generateKeypair(seed_res.value());
          auto &&[pub, priv] = suite.decomposeKeypair(kp);
          if (pub == pk) {
            SL_TRACE(logger_, "Key is correct {}", pk.toHex());
            res.emplace_back(std::move(pk));
          }
        }
      }
      std::move(
          cached_keys.begin(), cached_keys.end(), std::back_inserter(res));
      return res;
    }

    template <typename CryptoSuite>
    outcome::result<typename CryptoSuite::Keypair> generateKeypair(
        std::string_view mnemonic_phrase, const CryptoSuite &suite) {
      using Seed = typename CryptoSuite::Seed;
      OUTCOME_TRY(bip_seed, bip39_provider_->generateSeed(mnemonic_phrase));
      if (bip_seed.size() < Seed::size()) {
        return CryptoStoreError::WRONG_SEED_SIZE;
      }
      auto seed_span = gsl::make_span(bip_seed.data(), Seed::size());
      OUTCOME_TRY(seed, Seed::fromSpan(seed_span));
      return suite.generateKeypair(seed);
    }

    template <typename Suite>
    KeyCache<Suite> &getCache(
        std::shared_ptr<Suite> suite,
        std::unordered_map<KeyTypeId, KeyCache<Suite>> &caches,
        KeyTypeId type) const {
      auto it = caches.find(type);
      if (it == caches.end()) {
        auto &&[new_it, success] = caches.insert({type, KeyCache{type, suite}});
        BOOST_ASSERT(success);
        it = new_it;
      }
      return it->second;
    }

    libp2p::crypto::KeyPair ed25519KeyToLibp2pKeypair(
        const Ed25519Keypair &kp) const;

    mutable std::unordered_map<KeyTypeId, KeyCache<Ed25519Suite>> ed_caches_;
    mutable std::unordered_map<KeyTypeId, KeyCache<Sr25519Suite>> sr_caches_;
    std::shared_ptr<KeyFileStorage> file_storage_;
    std::shared_ptr<Ed25519Suite> ed_suite_;
    std::shared_ptr<Sr25519Suite> sr_suite_;
    std::shared_ptr<Bip39Provider> bip39_provider_;
    log::Logger logger_;
  };

}  // namespace kagome::crypto

OUTCOME_HPP_DECLARE_ERROR(kagome::crypto, CryptoStoreError);

#endif  // KAGOME_CRYPTO_STORE_IMPL_HPP
