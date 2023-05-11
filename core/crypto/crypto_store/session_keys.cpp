/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/crypto_store/session_keys.hpp"

#include "application/app_configuration.hpp"
#include "crypto/crypto_store.hpp"

namespace kagome::crypto {
  template <typename T,
            outcome::result<std::vector<decltype(T::public_key)>> (
                CryptoStore::*list_public)(KeyTypeId) const,
            outcome::result<T> (CryptoStore::*get_private)(
                KeyTypeId, const decltype(T::public_key) &) const,
            typename A,
            typename Eq>
  SessionKeys::Result<T> SessionKeysImpl::find(
      std::shared_ptr<T> &cache,
      KeyTypeId type,
      const std::vector<A> &authorities,
      const Eq &eq) {
    if (not roles_.flags.authority) {
      return std::nullopt;
    }
    if (cache) {
      auto it = std::find_if(
          authorities.begin(), authorities.end(), [&](const A &authority) {
            return eq(cache->public_key, authority);
          });

      if (it != authorities.end()) {
        return std::make_pair(cache, it - authorities.begin());
      }
    }
    auto keys_res = ((*store_).*list_public)(type);
    if (not keys_res) {
      return std::nullopt;
    }
    auto &keys = keys_res.value();
    for (auto &key : keys) {
      auto it = std::find_if(
          authorities.begin(), authorities.end(), [&](const A &authority) {
            return eq(key, authority);
          });
      if (it == authorities.end()) {
        continue;
      }
      auto keypair_res = store_->findSr25519Keypair(type, key);
      if (not keypair_res) {
        continue;
      }
      auto &keypair = keypair_res.value();
      cache = std::make_shared<T>(keypair);
      return std::make_pair(cache, it - authorities.begin());
    }
    return std::nullopt;
  }

  SessionKeysImpl::SessionKeysImpl(std::shared_ptr<CryptoStore> store,
                                   const application::AppConfiguration &config)
      : roles_(config.roles()), store_(store) {
    if (auto dev = config.devMnemonicPhrase()) {
      store_->generateEd25519Keypair(KEY_TYPE_GRAN, *dev).value();
      store_->generateSr25519Keypair(KEY_TYPE_BABE, *dev).value();
      store_->generateSr25519Keypair(KEY_TYPE_IMON, *dev).value();
      store_->generateSr25519Keypair(KEY_TYPE_AUDI, *dev).value();
      store_->generateSr25519Keypair(KEY_TYPE_ASGN, *dev).value();
      store_->generateSr25519Keypair(KEY_TYPE_PARA, *dev).value();
    }
  }

  SessionKeys::Result<Sr25519Keypair> SessionKeysImpl::getBabeKeyPair(
      const primitives::AuthorityList &authorities) {
    return find<Sr25519Keypair,
                &CryptoStore::getSr25519PublicKeys,
                &CryptoStore::findSr25519Keypair>(
        babe_key_pair_,
        KEY_TYPE_BABE,
        authorities,
        [](const Sr25519PublicKey &l, const primitives::Authority &r) {
          return l == r.id.id;
        });
  }

  const std::shared_ptr<Ed25519Keypair> &SessionKeysImpl::getGranKeyPair() {
    if (!gran_key_pair_ && roles_.flags.authority) {
      auto keys = store_->getEd25519PublicKeys(KEY_TYPE_GRAN);
      if (keys and not keys.value().empty()) {
        auto kp = store_->findEd25519Keypair(KEY_TYPE_GRAN, keys.value().at(0));
        gran_key_pair_ = std::make_shared<Ed25519Keypair>(kp.value());
      }
    }
    return gran_key_pair_;
  }

  SessionKeys::Result<Sr25519Keypair> SessionKeysImpl::getParaKeyPair(
      const std::vector<Sr25519PublicKey> &authorities) {
    return find<Sr25519Keypair,
                &CryptoStore::getSr25519PublicKeys,
                &CryptoStore::findSr25519Keypair>(
        para_key_pair_, KEY_TYPE_PARA, authorities, std::equal_to{});
  }

  std::shared_ptr<Sr25519Keypair> SessionKeysImpl::getAudiKeyPair(
      const std::vector<primitives::AuthorityDiscoveryId> &authorities) {
    if (auto res = find<Sr25519Keypair,
                        &CryptoStore::getSr25519PublicKeys,
                        &CryptoStore::findSr25519Keypair>(
            audi_key_pair_, KEY_TYPE_AUDI, authorities, std::equal_to{})) {
      return std::move(res->first);
    }
    return nullptr;
  }

}  // namespace kagome::crypto
