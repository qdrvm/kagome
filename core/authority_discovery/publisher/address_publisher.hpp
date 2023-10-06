/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "application/app_state_manager.hpp"
#include "blockchain/block_tree.hpp"
#include "crypto/crypto_store/session_keys.hpp"
#include "crypto/ed25519_provider.hpp"
#include "crypto/sr25519_provider.hpp"
#include "log/logger.hpp"
#include "runtime/runtime_api/authority_discovery_api.hpp"

#include <libp2p/crypto/key_marshaller.hpp>
#include <libp2p/host/host.hpp>
#include <libp2p/protocol/kademlia/kademlia.hpp>
#include <memory>

namespace kagome::authority_discovery {
  /**
   * Publishes listening addresses for authority discovery.
   * Authority discovery public key is used for Kademlia DHT key.
   */
  class AddressPublisher
      : public std::enable_shared_from_this<AddressPublisher> {
   public:
    AddressPublisher(
        std::shared_ptr<runtime::AuthorityDiscoveryApi> authority_discovery_api,
        network::Roles roles,
        std::shared_ptr<application::AppStateManager> app_state_manager,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<crypto::SessionKeys> keys,
        const libp2p::crypto::KeyPair &libp2p_key,
        const libp2p::crypto::marshaller::KeyMarshaller &key_marshaller,
        std::shared_ptr<crypto::Ed25519Provider> ed_crypto_provider,
        std::shared_ptr<crypto::Sr25519Provider> sr_crypto_provider,
        libp2p::Host &host,
        std::shared_ptr<libp2p::protocol::kademlia::Kademlia> kademlia,
        std::shared_ptr<libp2p::basic::Scheduler> scheduler);

    bool start();

    outcome::result<void> publishOwnAddress();

   private:
    std::shared_ptr<runtime::AuthorityDiscoveryApi> authority_discovery_api_;
    network::Roles roles_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;

    std::shared_ptr<crypto::SessionKeys> keys_;

    std::shared_ptr<crypto::Ed25519Provider> ed_crypto_provider_;
    std::shared_ptr<crypto::Sr25519Provider> sr_crypto_provider_;

    libp2p::Host &host_;
    std::shared_ptr<libp2p::protocol::kademlia::Kademlia> kademlia_;

    std::shared_ptr<libp2p::basic::Scheduler> scheduler_;

    log::Logger log_;

    std::optional<crypto::Ed25519Keypair> libp2p_key_;
    std::optional<libp2p::crypto::ProtobufKey> libp2p_key_pb_;
  };
}  // namespace kagome::authority_discovery
