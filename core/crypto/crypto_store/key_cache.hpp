/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>

#include <optional>

#include "crypto/crypto_store/key_type.hpp"

namespace kagome::crypto {

  /**
   * In-memory cache of keys belonging to the same crypto suite and the same key
   * type
   * @see crypto_suites.hpp
   * @see key_type.hpp
   */
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
      // this should be refactored in the future, the session key should be
      // determined by either node's config or node's internal logic
      if (not session_key_) {
        session_key_ = suite_->composeKeypair(pubkey, privkey);
      }
      cache_.emplace(std::move(pubkey), std::move(privkey));
    }

    /**
     * Session keys are short-living keys used by the node
     */
    const std::optional<Keypair> &getSessionKey() const noexcept {
      return session_key_;
    }

    std::unordered_set<PublicKey> getPublicKeys() const {
      std::unordered_set<PublicKey> keys;
      for (auto &&[k, v] : cache_) {
        keys.emplace(k);
      }
      return keys;
    }

    std::optional<Keypair> searchKeypair(const PublicKey &key) const {
      auto it = cache_.find(key);
      if (it != cache_.end()) {
        return suite_->composeKeypair(it->first, it->second);
      }
      return std::nullopt;
    }

   private:
    KeyTypeId type_;
    std::optional<Keypair> session_key_;
    std::unordered_map<PublicKey, PrivateKey> cache_;
    std::shared_ptr<CryptoSuite> suite_;
  };
}  // namespace kagome::crypto
