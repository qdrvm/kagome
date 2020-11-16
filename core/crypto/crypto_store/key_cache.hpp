/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_KEY_CACHE_HPP
#define KAGOME_KEY_CACHE_HPP

#include <unordered_map>
#include <unordered_set>
#include <memory>

#include <boost/optional.hpp>

#include "crypto/crypto_store/key_type.hpp"

namespace kagome::crypto {

  template <typename CryptoSuite>
  class KeyCache {
   public:
    using PublicKey = typename CryptoSuite::PublicKey;
    using PrivateKey = typename CryptoSuite::PrivateKey;
    using Keypair = typename CryptoSuite::Keypair;
    using Seed = typename CryptoSuite::Seed;

    explicit KeyCache(KeyTypeId type, std::shared_ptr<CryptoSuite> suite)
        : type_{type}, suite_{std::move(suite)} {
      BOOST_ASSERT(suite_ != nullptr);
    }

    void insert(PublicKey pubkey, PrivateKey privkey) {
      cache_.emplace(std::move(pubkey), std::move(privkey));
    }

    boost::optional<Keypair> const &getSessionKey() const noexcept {
      return session_key_;
    }

    std::unordered_set<PublicKey> getPublicKeys() const {
      std::unordered_set<PublicKey> keys;
      for (auto &&[k, v] : cache_) {
        keys.emplace(k);
      }
      return keys;
    }

    boost::optional<Keypair> searchKeypair(const PublicKey &key) const {
      auto it = cache_.find(key);
      if (it != cache_.end()) {
        return suite_->composeKeypair(it->first, it->second);
      }
      return boost::none;
    }

   private:
    KeyTypeId type_;
    boost::optional<Keypair> session_key_;
    std::unordered_map<PublicKey, PrivateKey> cache_;
    std::shared_ptr<CryptoSuite> suite_;
  };
}

#endif  // KAGOME_KEY_CACHE_HPP
