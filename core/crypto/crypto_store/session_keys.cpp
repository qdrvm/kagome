/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/crypto_store/session_keys.hpp"

#include "application/app_configuration.hpp"
#include "crypto/crypto_store.hpp"

namespace kagome::crypto {
  template <typename T,
            SessionKeysImpl::FnListPublic<T> list_public,
            SessionKeysImpl::FnGetPrivate<T> get_private,
            typename A,
            typename Eq>
  SessionKeys::KeypairWithIndexOpt<T> SessionKeysImpl::find(
      KeypairWithIndexOpt<T> &cache,
      KeyType type,
      const std::vector<A> &authorities,
      const Eq &eq) {
    if (not roles_.flags.authority) {
      return std::nullopt;
    }
    if (cache) {
      if (cache->second < authorities.size()
          && eq(cache->first->public_key, authorities[cache->second])) {
        return cache;
      }
      auto it = std::find_if(
          authorities.begin(), authorities.end(), [&](const A &authority) {
            return eq(cache->first->public_key, authority);
          });
      if (it != authorities.end()) {
        cache->second = it - authorities.begin();
        return cache;
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
      auto keypair_res = ((*store_).*get_private)(type, key);
      if (not keypair_res) {
        continue;
      }
      auto &keypair = keypair_res.value();
      cache.emplace(std::make_shared<T>(keypair), it - authorities.begin());
      return cache;
    }
    return std::nullopt;
  }

  SessionKeysImpl::SessionKeysImpl(std::shared_ptr<CryptoStore> store,
                                   const application::AppConfiguration &config)
      : roles_(config.roles()), store_(store) {
    if (auto dev = config.devMnemonicPhrase()) {
      store_->generateEd25519Keypair(KeyTypes::GRANDPA, *dev).value();
      store_->generateSr25519Keypair(KeyTypes::BABE, *dev).value();
      store_->generateSr25519Keypair(KeyTypes::IM_ONLINE, *dev).value();
      store_->generateSr25519Keypair(KeyTypes::AUTHORITY_DISCOVERY, *dev)
          .value();
      store_->generateSr25519Keypair(KeyTypes::KEY_TYPE_ASGN, *dev).value();
      store_->generateSr25519Keypair(KeyTypes::KEY_TYPE_PARA, *dev).value();
      store_->generateEcdsaKeypair(KeyTypes::BEEFY, *dev).value();
    }
  }

  SessionKeys::KeypairWithIndexOpt<Sr25519Keypair>
  SessionKeysImpl::getBabeKeyPair(
      const primitives::AuthorityList &authorities) {
    return find<Sr25519Keypair,
                &CryptoStore::getSr25519PublicKeys,
                &CryptoStore::findSr25519Keypair>(
        babe_key_pair_,
        KeyTypes::BABE,
        authorities,
        [](const Sr25519PublicKey &l, const primitives::Authority &r) {
          return l == r.id.id;
        });
  }

  SessionKeys::KeypairWithIndexOpt<Sr25519Keypair>
  SessionKeysImpl::getSassafrasKeyPair(
      const primitives::AuthorityList &authorities) {
    return find<Sr25519Keypair,
                &CryptoStore::getSr25519PublicKeys,
                &CryptoStore::findSr25519Keypair>(
        babe_key_pair_,
        KeyTypes::SASSAFRAS,
        authorities,
        [](const Sr25519PublicKey &l, const primitives::Authority &r) {
          return l == r.id.id;
        });
  }

  std::shared_ptr<Ed25519Keypair> SessionKeysImpl::getGranKeyPair(
      const primitives::AuthoritySet &authorities) {
    if (auto res = find<Ed25519Keypair,
                        &CryptoStore::getEd25519PublicKeys,
                        &CryptoStore::findEd25519Keypair>(
            gran_key_pair_,
            KeyTypes::GRANDPA,
            authorities.authorities,
            [](const Ed25519PublicKey &l, const primitives::Authority &r) {
              return l == r.id.id;
            })) {
      return std::move(res->first);
    }
    return nullptr;
  }

  SessionKeys::KeypairWithIndexOpt<Sr25519Keypair>
  SessionKeysImpl::getParaKeyPair(
      const std::vector<Sr25519PublicKey> &authorities) {
    return find<Sr25519Keypair,
                &CryptoStore::getSr25519PublicKeys,
                &CryptoStore::findSr25519Keypair>(
        para_key_pair_, KeyTypes::KEY_TYPE_PARA, authorities, std::equal_to{});
  }

  std::shared_ptr<Sr25519Keypair> SessionKeysImpl::getAudiKeyPair(
      const std::vector<primitives::AuthorityDiscoveryId> &authorities) {
    if (auto res = find<Sr25519Keypair,
                        &CryptoStore::getSr25519PublicKeys,
                        &CryptoStore::findSr25519Keypair>(
            audi_key_pair_,
            KeyTypes::AUTHORITY_DISCOVERY,
            authorities,
            std::equal_to{})) {
      return std::move(res->first);
    }
    return nullptr;
  }

  SessionKeys::KeypairWithIndexOpt<EcdsaKeypair>
  SessionKeysImpl::getBeefKeyPair(
      const std::vector<EcdsaPublicKey> &authorities) {
    return find<EcdsaKeypair,
                &CryptoStore::getEcdsaPublicKeys,
                &CryptoStore::findEcdsaKeypair>(
        beef_key_pair_, KeyTypes::BEEFY, authorities, std::equal_to{});
  }
}  // namespace kagome::crypto
