/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ADDRESS_PUBLISHER_IMPL_HPP
#define KAGOME_ADDRESS_PUBLISHER_IMPL_HPP

#include "application/app_state_manager.hpp"
#include "blockchain/block_tree.hpp"
#include "crypto/crypto_store.hpp"
#include "crypto/crypto_store/session_keys.hpp"
#include "crypto/ed25519_provider.hpp"
#include "crypto/sr25519_provider.hpp"
#include "runtime/runtime_api/authority_discovery_api.hpp"

#include <libp2p/crypto/key_marshaller.hpp>
#include <libp2p/host/host.hpp>
#include <libp2p/protocol/kademlia/kademlia.hpp>
#include <memory>

#include "log/logger.hpp"

namespace kagome::authority_discovery {

  class AddressPublisherImpl
      : public std::enable_shared_from_this<AddressPublisherImpl> {
   public:
    AddressPublisherImpl(
        std::shared_ptr<runtime::AuthorityDiscoveryApi> authority_discovery_api,
        std::shared_ptr<application::AppStateManager> app_state_manager,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<crypto::SessionKeys> keys,
        const libp2p::crypto::KeyPair &libp2p_key,
        const libp2p::crypto::marshaller::KeyMarshaller &key_marshaller,
        std::shared_ptr<crypto::Ed25519Provider> crypto_provider,
        std::shared_ptr<crypto::Sr25519Provider> crypto_provider2,
        libp2p::Host &host,
        std::shared_ptr<libp2p::protocol::kademlia::Kademlia> kademlia,
        std::shared_ptr<libp2p::basic::Scheduler> scheduler);

    bool start();

   private:
    outcome::result<void> publishOwnAddress();

    std::shared_ptr<runtime::AuthorityDiscoveryApi> authority_discovery_api_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;

    std::shared_ptr<crypto::SessionKeys> keys_;

    std::shared_ptr<crypto::Ed25519Provider> crypto_provider_;
    std::shared_ptr<crypto::Sr25519Provider> crypto_provider2_;

    libp2p::Host &host_;
    std::shared_ptr<libp2p::protocol::kademlia::Kademlia> kademlia_;

    std::shared_ptr<libp2p::basic::Scheduler> scheduler_;

    log::Logger log_;

    std::optional<crypto::Ed25519Keypair> libp2p_key_;
    std::optional<libp2p::crypto::ProtobufKey> libp2p_key_pb_;
  };

}  // namespace kagome::authority_discovery

#endif  // KAGOME_ADDRESS_PUBLISHER_IMPL_HPP
