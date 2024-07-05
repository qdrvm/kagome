/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <fstream>

#include "application/app_configuration.hpp"
#include "application/chain_spec.hpp"
#include "common/bytestr.hpp"
#include "common/outcome_throw.hpp"
#include "crypto/ed25519_provider.hpp"
#include "crypto/key_store.hpp"
#include "crypto/random_generator.hpp"

namespace kagome::injector {
  inline std::shared_ptr<libp2p::crypto::KeyPair> get_peer_keypair(
      const application::AppConfiguration &app_config,
      const application::ChainSpec &chain,
      const crypto::Ed25519Provider &crypto_provider,
      crypto::CSPRNG &csprng,
      crypto::KeyStore &key_store) {
    auto log = log::createLogger("Injector", "injector");

    if (app_config.nodeKey()) {
      log->info("Will use LibP2P keypair from config or 'node-key' CLI arg");

      auto provided_keypair =
          crypto_provider.generateKeypair(*app_config.nodeKey(), {}).value();
      BOOST_ASSERT(provided_keypair.secret_key == app_config.nodeKey().value());

      auto key_pair = std::make_shared<libp2p::crypto::KeyPair>(
          crypto::ed25519KeyToLibp2pKeypair(provided_keypair));

      return key_pair;
    }

    if (app_config.nodeKeyFile()) {
      const auto &path = app_config.nodeKeyFile().value();
      log->info(
          "Will use LibP2P keypair from config or 'node-key-file' CLI arg");
      auto key = key_store.loadLibp2pKeypair(path);
      if (key.has_error()) {
        log->error("Unable to load user provided key from {}. Error: {}",
                   path,
                   key.error());
        common::raise(key.error());
      } else {
        auto key_pair =
            std::make_shared<libp2p::crypto::KeyPair>(std::move(key.value()));
        return key_pair;
      }
    }

    auto path = app_config.chainPath(chain.id()) / "network/secret_ed25519";
    if (auto r = key_store.loadLibp2pKeypair(path)) {
      log->info(
          "Will use LibP2P keypair from config or args (loading from base "
          "path)");

      auto &stored_keypair = r.value();

      auto key_pair =
          std::make_shared<libp2p::crypto::KeyPair>(std::move(stored_keypair));

      return key_pair;
    }

    log->warn(
        "Can not obtain a libp2p keypair from crypto storage. "
        "A unique one will be generated");

    crypto::SecureBuffer<> seed_buf(crypto::Ed25519Seed::size());
    csprng.fillRandomly(seed_buf);
    // SAFETY: buffer is initialized with seed's size
    auto seed = crypto::Ed25519Seed::from(std::move(seed_buf)).value();
    auto generated_keypair = crypto_provider.generateKeypair(seed, {}).value();
    auto save = app_config.shouldSaveNodeKey();
    if (save) {
      std::ofstream file{path.c_str()};
      file.write(byte2str(seed.unsafeBytes()).data(), seed.size());
    }

    auto key_pair = std::make_shared<libp2p::crypto::KeyPair>(
        crypto::ed25519KeyToLibp2pKeypair(generated_keypair));

    return key_pair;
  }
}  // namespace kagome::injector
