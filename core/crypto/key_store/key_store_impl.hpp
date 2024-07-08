/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <unordered_set>

#include "common/blob.hpp"
#include "common/optref.hpp"
#include "crypto/bip39/bip39_provider.hpp"
#include "crypto/bip39/mnemonic.hpp"
#include "crypto/key_store.hpp"
#include "crypto/key_store/key_file_storage.hpp"
#include "crypto/random_generator.hpp"
#include "filesystem/common.hpp"
#include "log/logger.hpp"
#include "outcome/outcome.hpp"
#include "utils/read_file.hpp"

namespace kagome::crypto {

  template <Suite T>
  class KeySuiteStoreImpl final : public KeySuiteStore<T> {
   public:
    using Keypair = typename T::Keypair;
    using PrivateKey = typename T::PrivateKey;
    using PublicKey = typename T::PublicKey;
    using Seed = typename T::Seed;

    KeySuiteStoreImpl(std::shared_ptr<T> suite,
                      std::shared_ptr<Bip39Provider> bip39_provider,
                      std::shared_ptr<CSPRNG> csprng,
                      std::shared_ptr<KeyFileStorage> key_fs)
        : suite_{std::move(suite)},
          file_storage_{std::move(key_fs)},
          bip39_provider_{std::move(bip39_provider)},
          csprng_{std::move(csprng)},
          logger_{log::createLogger("KeyStore", "key_store")} {
      BOOST_ASSERT(suite_ != nullptr);
      BOOST_ASSERT(bip39_provider_ != nullptr);
      BOOST_ASSERT(file_storage_ != nullptr);
      BOOST_ASSERT(csprng_ != nullptr);
    }

    outcome::result<Keypair> generateKeypair(
        KeyType key_type, std::string_view mnemonic_phrase) override {
      OUTCOME_TRY(bip, bip39_provider_->generateSeed(mnemonic_phrase));
      auto seed = Seed::from(bip.seed);
      OUTCOME_TRY(kp, suite_->generateKeypair(seed, bip.junctions));
      keys_[key_type].insert(std::pair{kp.public_key, kp});
      return kp;
    }

    outcome::result<Keypair> generateKeypair(KeyType key_type,
                                             const Seed &seed) override {
      OUTCOME_TRY(kp, suite_->generateKeypair(seed, {}));
      keys_[key_type].insert(std::pair{kp.public_key, kp});
      return kp;
    }

    outcome::result<Keypair> generateKeypairOnDisk(KeyType key_type) override {
      SecureBuffer<> seed_buf(Seed::size());
      csprng_->fillRandomly(seed_buf);
      OUTCOME_TRY(seed, Seed::from(std::move(seed_buf)));
      OUTCOME_TRY(kp, suite_->generateKeypair(seed, {}));
      keys_[key_type].insert(std::pair{kp.public_key, kp});
      OUTCOME_TRY(file_storage_->saveKeyPair(
          key_type, kp.public_key, seed.unsafeBytes()));
      return kp;
    }

    OptRef<const Keypair> findKeypair(KeyType key_type,
                                      const PublicKey &pk) const override {
      auto keys_it = keys_.find(key_type);
      if (keys_it == keys_.end()) {
        return std::nullopt;
      }
      if (auto it = keys_it->second.find(pk); it != keys_it->second.end()) {
        return it->second;
      }
      return std::nullopt;
    }

    outcome::result<std::vector<PublicKey>> getPublicKeys(
        KeyType key_type) const override {
      auto keys_it = keys_.find(key_type);
      if (keys_it == keys_.end()) {
        return std::vector<PublicKey>{};
      }
      auto &keys = keys_it->second;
      std::vector<PublicKey> res;
      res.reserve(keys.size());
      for (auto &[public_key, private_key] : keys) {
        res.push_back(public_key);
      }
      return res;
    }

   private:
    std::shared_ptr<T> suite_;
    std::shared_ptr<KeyFileStorage> file_storage_;
    std::shared_ptr<Bip39Provider> bip39_provider_;
    std::shared_ptr<CSPRNG> csprng_;
    std::unordered_map<KeyType, std::unordered_map<PublicKey, Keypair>> keys_;
    log::Logger logger_;
  };

}  // namespace kagome::crypto
