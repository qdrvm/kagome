/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

#include <libp2p/crypto/key_marshaller.hpp>
#include <libp2p/peer/peer_id.hpp>
#include "crypto/ed25519_provider.hpp"
#include "crypto/key_store.hpp"
#include "crypto/random_generator/boost_generator.hpp"

namespace kagome::key {

  class Key {
   public:
    Key(std::shared_ptr<crypto::Ed25519Provider> ed_crypto_provider,
        std::shared_ptr<libp2p::crypto::marshaller::KeyMarshaller>
            key_marshaller)
        : ed_crypto_provider_{std::move(ed_crypto_provider)},
          key_marshaller_{std::move(key_marshaller)} {
      BOOST_ASSERT(ed_crypto_provider_ != nullptr);
      BOOST_ASSERT(key_marshaller_ != nullptr);
    }

    outcome::result<void> run() {
      auto random_generator = std::make_unique<crypto::BoostRandomGenerator>();
      OUTCOME_TRY(
          seed,
          crypto::Ed25519Seed::from(crypto::SecureCleanGuard{
              random_generator->randomBytes(crypto::Ed25519Seed::size())}));
      OUTCOME_TRY(keypair, ed_crypto_provider_->generateKeypair(seed, {}));
      const auto libp2p_key = crypto::ed25519KeyToLibp2pKeypair(keypair);
      const libp2p::crypto::ProtobufKey protobuf_key{
          key_marshaller_->marshal(libp2p_key.publicKey).value()};
      auto peer_id = libp2p::peer::PeerId::fromPublicKey(protobuf_key);
      if (not peer_id) {
        return peer_id.error();
      }
      std::cerr << peer_id.value().toBase58() << "\n";
      std::cout << kagome::common::hex_lower(keypair.secret_key.unsafeBytes())
                << "\n";

      return outcome::success();
    }

   private:
    std::shared_ptr<crypto::Ed25519Provider> ed_crypto_provider_;
    std::shared_ptr<libp2p::crypto::marshaller::KeyMarshaller> key_marshaller_;
  };
}  // namespace kagome::key
