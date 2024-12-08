/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */
#include <libp2p/crypto/key_marshaller.hpp>
#include <libp2p/peer/peer_id.hpp>
#include "crypto/ed25519/ed25519_provider_impl.hpp"
#include "crypto/hasher/hasher_impl.hpp"
#include "crypto/key_store.hpp"
#include "crypto/random_generator/boost_generator.hpp"

namespace kagome {
  int key_main(int argc, const char **argv) {
    if (argc == 2) {
      if (std::string(argv[1]) == "--generate-node-key") {
        auto random_generator =
            std::make_shared<crypto::BoostRandomGenerator>();
        auto hasher = std::make_shared<crypto::HasherImpl>();

        auto ed25519_provider =
            std::make_shared<kagome::crypto::Ed25519ProviderImpl>(hasher);

        auto seed = kagome::crypto::Ed25519Seed::from(
                        crypto::SecureCleanGuard{random_generator->randomBytes(
                            kagome::crypto::Ed25519Seed::size())})
                        .value();
        auto keypair = ed25519_provider->generateKeypair(seed, {}).value();
        auto libp2p_key = crypto::ed25519KeyToLibp2pKeypair(keypair);
        libp2p::crypto::ProtobufKey protobuf_key{
            common::Buffer{libp2p_key.publicKey.data}};
        auto peer_id = libp2p::peer::PeerId::fromPublicKey(protobuf_key);
        std::cerr << peer_id.value().toBase58() << std::endl;
        std::cout << kagome::common::hex_lower(keypair.secret_key.unsafeBytes())
                  << std::endl;
      } else if (std::string(argv[1]) == "--help") {
        std::cerr << "Usage: " << argv[0] << " --generate-node-key"
                  << "\nGenerates a node key and prints the peer ID to stderr "
                     "and the secret key to stdout."
                  << std::endl;
      } else {
        std::cerr << "Unknown command: " << argv[1] << std::endl;
        std::cerr << "Usage: " << argv[0] << " --generate-node-key"
                  << std::endl;
        return 2;
      }
    } else {
      std::cerr << "Usage: " << argv[0] << " --generate-node-key" << std::endl;
      return 1;
    }
    return 0;
  }

}  // namespace kagome
