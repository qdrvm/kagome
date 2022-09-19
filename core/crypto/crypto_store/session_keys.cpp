/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/crypto_store/session_keys.hpp"

#include "crypto/crypto_store.hpp"

namespace kagome::crypto {

  SessionKeys::SessionKeys(std::shared_ptr<CryptoStore> store,
                           const network::Roles &roles)
      : roles_(roles), store_(store) {}

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
