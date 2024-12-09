/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "key/key.hpp"

#include <libp2p/peer/peer_id.hpp>
#include "crypto/key_store.hpp"
#include "crypto/random_generator/boost_generator.hpp"

namespace kagome::key {

  Key::Key(
      std::shared_ptr<crypto::Ed25519Provider> ed_crypto_provider,
      std::shared_ptr<libp2p::crypto::marshaller::KeyMarshaller> key_marshaller)
      : ed_crypto_provider_{std::move(ed_crypto_provider)},
        key_marshaller_{std::move(key_marshaller)} {
    BOOST_ASSERT(ed_crypto_provider_ != nullptr);
    BOOST_ASSERT(key_marshaller_ != nullptr);
  }

  outcome::result<void> Key::run() {
    auto random_generator = std::make_shared<crypto::BoostRandomGenerator>();

    auto seed = kagome::crypto::Ed25519Seed::from(
                    crypto::SecureCleanGuard{random_generator->randomBytes(
                        kagome::crypto::Ed25519Seed::size())})
                    .value();
    auto keypair = ed_crypto_provider_->generateKeypair(seed, {}).value();
    auto libp2p_key = crypto::ed25519KeyToLibp2pKeypair(keypair);
    libp2p::crypto::ProtobufKey protobuf_key{
        key_marshaller_->marshal(libp2p_key.publicKey).value()};
    auto peer_id = libp2p::peer::PeerId::fromPublicKey(protobuf_key);
    std::cerr << peer_id.value().toBase58() << std::endl;
    std::cout << kagome::common::hex_lower(keypair.secret_key.unsafeBytes())
              << std::endl;

    return outcome::success();
  }
}  // namespace kagome::key
