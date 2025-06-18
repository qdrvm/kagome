/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/key_store/session_keys.hpp"

#include "application/app_configuration.hpp"
#include "crypto/ed25519_provider.hpp"
#include "crypto/key_store.hpp"

namespace kagome::crypto {
  template <Suite T, typename A, typename Eq>
  SessionKeys::KeypairWithIndexOpt<typename T::Keypair> SessionKeysImpl::find(
      KeypairWithIndexOpt<typename T::Keypair> &cache,
      KeyType type,
      const KeySuiteStore<T> &store,
      const std::vector<A> &authorities,
      const Eq &eq) {
    if (not roles_.isAuthority()) {
      return std::nullopt;
    }
    if (cache) {
      if (cache->second < authorities.size()
          && eq(cache->first->public_key, authorities[cache->second])) {
        return cache;
      }
      auto it = std::ranges::find_if(
          authorities.begin(), authorities.end(), [&](const A &authority) {
            return eq(cache->first->public_key, authority);
          });
      if (it != authorities.end()) {
        cache->second = it - authorities.begin();
        return cache;
      }
    }
    auto keys_res = store.getPublicKeys(type);
    if (not keys_res) {
      return std::nullopt;
    }
    auto &keys = keys_res.value();
    for (auto &pubkey : keys) {
      auto it = std::ranges::find_if(
          authorities.begin(), authorities.end(), [&](const A &authority) {
            return eq(pubkey, authority);
          });
      if (it == authorities.end()) {
        continue;
      }
      auto keypair_opt = store.findKeypair(type, pubkey);
      if (not keypair_opt) {
        continue;
      }
      cache.emplace(
          std::make_shared<typename T::Keypair>(std::move(keypair_opt.value())),
          it - authorities.begin());
      return cache;
    }
    return std::nullopt;
  }

  SessionKeysImpl::SessionKeysImpl(std::shared_ptr<KeyStore> store,
                                   const application::AppConfiguration &config)
      : roles_(config.roles()), store_(std::move(store)) {
    if (auto dev = config.devMnemonicPhrase()) {
      // Ed25519
      store_->ed25519().generateKeypair(KeyTypes::GRANDPA, *dev).value();
      // Sr25519
      for (auto key_type : {KeyTypes::BABE,
                            KeyTypes::IM_ONLINE,
                            KeyTypes::AUTHORITY_DISCOVERY,
                            KeyTypes::ASSIGNMENT,
                            KeyTypes::PARACHAIN}) {
        store_->sr25519().generateKeypair(key_type, *dev).value();
      }
      // Ecdsa
      store_->ecdsa().generateKeypair(KeyTypes::BEEFY, *dev).value();
    }
  }

  SessionKeys::KeypairWithIndexOpt<Sr25519Keypair>
  SessionKeysImpl::getBabeKeyPair(
      const consensus::babe::Authorities &authorities) {
    return find<Sr25519Provider>(
        babe_key_pair_,
        KeyTypes::BABE,
        store_->sr25519(),
        authorities,
        [](const Sr25519PublicKey &l, const consensus::babe::Authority &r) {
          return l == r.id;
        });
  }

  // SessionKeys::KeypairWithIndexOpt<BandersnatchKeypair>
  // SessionKeysImpl::getSassafrasKeyPair(
  //     const consensus::sassafras::Authorities &authorities) {
  //   return find<BandersnatchKeypair,
  //               &KeyStore::getBandersnatchPublicKeys,
  //               &KeyStore::findBandersnatchKeypair>(
  //       sass_key_pair_,
  //       KeyTypes::SASSAFRAS,
  //       authorities,
  //       [](const BandersnatchPublicKey &l,
  //          const consensus::sassafras::Authority &r) {
  //         return l == r;
  //       });
  // }

  std::shared_ptr<Ed25519Keypair> SessionKeysImpl::getGranKeyPair(
      const consensus::grandpa::AuthoritySet &authorities) {
    if (auto res = find<Ed25519Provider>(
            gran_key_pair_,
            KeyTypes::GRANDPA,
            store_->ed25519(),
            authorities.authorities,
            [](const Ed25519PublicKey &l,
               const consensus::grandpa::Authority &r) { return l == r.id; })) {
      return std::move(res->first);
    }
    return nullptr;
  }

  SessionKeys::KeypairWithIndexOpt<Sr25519Keypair>
  SessionKeysImpl::getParaKeyPair(
      const std::vector<Sr25519PublicKey> &authorities) {
    return find<Sr25519Provider>(para_key_pair_,
                                 KeyTypes::PARACHAIN,
                                 store_->sr25519(),
                                 authorities,
                                 std::equal_to{});
  }

  std::shared_ptr<Sr25519Keypair> SessionKeysImpl::getAudiKeyPair(
      const std::vector<primitives::AuthorityDiscoveryId> &authorities) {
    if (auto res = find<Sr25519Provider>(audi_key_pair_,
                                         KeyTypes::AUTHORITY_DISCOVERY,
                                         store_->sr25519(),
                                         authorities,
                                         std::equal_to{})) {
      return std::move(res->first);
    }
    return nullptr;
  }

  SessionKeys::KeypairWithIndexOpt<EcdsaKeypair>
  SessionKeysImpl::getBeefKeyPair(
      const std::vector<EcdsaPublicKey> &authorities) {
    return find<EcdsaProvider>(beef_key_pair_,
                               KeyTypes::BEEFY,
                               store_->ecdsa(),
                               authorities,
                               std::equal_to{});
  }

  std::vector<Sr25519Keypair> SessionKeysImpl::getAudiKeyPairs() {
    std::vector<Sr25519Keypair> keypairs;
    auto keys_res =
        store_->sr25519().getPublicKeys(KeyTypes::AUTHORITY_DISCOVERY);
    if (not keys_res || keys_res.value().empty()) {
      return keypairs;
    }

    for (const auto &pubkey : keys_res.value()) {
      auto keypair_opt =
          store_->sr25519().findKeypair(KeyTypes::AUTHORITY_DISCOVERY, pubkey);
      if (keypair_opt) {
        keypairs.push_back(keypair_opt.value());
      }
    }

    return keypairs;
  }

}  // namespace kagome::crypto
