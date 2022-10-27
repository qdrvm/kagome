/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CORE_INJECTOR_GET_PEER_KEYPAIR_HPP
#define KAGOME_CORE_INJECTOR_GET_PEER_KEYPAIR_HPP

#include "application/app_configuration.hpp"
#include "common/outcome_throw.hpp"
#include "crypto/crypto_store/crypto_store_impl.hpp"
#include "crypto/ed25519_provider.hpp"

namespace kagome::injector {
  inline const std::shared_ptr<libp2p::crypto::KeyPair> &get_peer_keypair(
      const application::AppConfiguration &app_config,
      const crypto::Ed25519Provider &crypto_provider,
      crypto::CryptoStore &crypto_store) {
    static auto initialized =
        std::optional<std::shared_ptr<libp2p::crypto::KeyPair>>(std::nullopt);

    if (initialized) {
      return initialized.value();
    }

    auto log = log::createLogger("Injector", "injector");

    if (app_config.nodeKey()) {
      log->info("Will use LibP2P keypair from config or 'node-key' CLI arg");

      auto provided_keypair = crypto_provider.generateKeypair(
          crypto::Ed25519Seed{app_config.nodeKey().value()});
      BOOST_ASSERT(provided_keypair.secret_key == app_config.nodeKey().value());

      auto key_pair = std::make_shared<libp2p::crypto::KeyPair>(
          crypto::ed25519KeyToLibp2pKeypair(provided_keypair));

      initialized.emplace(std::move(key_pair));
      return initialized.value();
    }

    if (app_config.nodeKeyFile()) {
      const auto &path = app_config.nodeKeyFile().value();
      log->info(
          "Will use LibP2P keypair from config or 'node-key-file' CLI arg");
      auto key = crypto_store.loadLibp2pKeypair(path);
      if (key.has_error()) {
        log->error("Unable to load user provided key from {}. Error: {}",
                   path,
                   key.error().message());
        common::raise(key.error());
      } else {
        auto key_pair =
            std::make_shared<libp2p::crypto::KeyPair>(std::move(key.value()));
        initialized.emplace(std::move(key_pair));
        return initialized.value();
      }
    }

    if (crypto_store.getLibp2pKeypair().has_value()) {
      log->info(
          "Will use LibP2P keypair from config or args (loading from base "
          "path)");

      auto stored_keypair = crypto_store.getLibp2pKeypair().value();

      auto key_pair =
          std::make_shared<libp2p::crypto::KeyPair>(std::move(stored_keypair));

      initialized.emplace(std::move(key_pair));
      return initialized.value();
    }

    log->warn(
        "Can not obtain a libp2p keypair from crypto storage. "
        "A unique one will be generated");

    kagome::crypto::Ed25519Keypair generated_keypair;
    auto save = app_config.shouldSaveNodeKey();
    if (save) {
      auto res = crypto_store.generateEd25519KeypairOnDisk(
          crypto::KnownKeyTypeId::KEY_TYPE_LP2P);
      if (res.has_error()) {
        log->warn("Can't save libp2p keypair: {}", res.error());
        save = false;
      } else {
        generated_keypair = res.value();
      }
    }
    if (not save) {
      generated_keypair = crypto_provider.generateKeypair();
    }

    auto key_pair = std::make_shared<libp2p::crypto::KeyPair>(
        crypto::ed25519KeyToLibp2pKeypair(generated_keypair));

    initialized.emplace(std::move(key_pair));
    return initialized.value();
  }
}  // namespace kagome::injector

#endif  // KAGOME_CORE_INJECTOR_GET_PEER_KEYPAIR_HPP
