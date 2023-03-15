/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/crypto_store/session_keys.hpp"

#include "application/app_configuration.hpp"
#include "crypto/crypto_store.hpp"

namespace kagome::crypto {

  SessionKeys::SessionKeys(std::shared_ptr<CryptoStore> store,
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

  const std::shared_ptr<Sr25519Keypair> &SessionKeys::getBabeKeyPair() {
    if (!babe_key_pair_ && roles_.flags.authority) {
      auto keys = store_->getSr25519PublicKeys(KEY_TYPE_BABE);
      if (keys and not keys.value().empty()) {
        auto kp = store_->findSr25519Keypair(KEY_TYPE_BABE, keys.value().at(0));
        babe_key_pair_ = std::make_shared<Sr25519Keypair>(kp.value());
      }
    }
    return babe_key_pair_;
  }

  const std::shared_ptr<Ed25519Keypair> &SessionKeys::getGranKeyPair() {
    if (!gran_key_pair_ && roles_.flags.authority) {
      auto keys = store_->getEd25519PublicKeys(KEY_TYPE_GRAN);
      if (keys and not keys.value().empty()) {
        auto kp = store_->findEd25519Keypair(KEY_TYPE_GRAN, keys.value().at(0));
        gran_key_pair_ = std::make_shared<Ed25519Keypair>(kp.value());
      }
    }
    return gran_key_pair_;
  }

  const std::shared_ptr<Sr25519Keypair> &SessionKeys::getParaKeyPair() {
    if (not para_key_pair_ && roles_.flags.authority) {
      auto keys = store_->getSr25519PublicKeys(KEY_TYPE_PARA);
      if (keys and not keys.value().empty()) {
        auto kp = store_->findSr25519Keypair(KEY_TYPE_PARA, keys.value().at(0));
        para_key_pair_ = std::make_shared<Sr25519Keypair>(kp.value());
      }
    }
    return para_key_pair_;
  }

  const std::shared_ptr<Sr25519Keypair> &SessionKeys::getAudiKeyPair() {
    if (!audi_key_pair_ && roles_.flags.authority) {
      auto keys = store_->getSr25519PublicKeys(KEY_TYPE_AUDI);
      if (keys and not keys.value().empty()) {
        auto kp = store_->findSr25519Keypair(KEY_TYPE_AUDI, keys.value().at(0));
        audi_key_pair_ = std::make_shared<Sr25519Keypair>(kp.value());
      }
    }
    return audi_key_pair_;
  }

}  // namespace kagome::crypto
