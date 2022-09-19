/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ADDRESS_PUBLISHER_IMPL_HPP
#define KAGOME_ADDRESS_PUBLISHER_IMPL_HPP

#include "authority_discovery/address_publisher.hpp"

#include "blockchain/block_tree.hpp"
#include "consensus/authority/authority_manager.hpp"
#include "crypto/crypto_store/session_keys.hpp"
#include "crypto/crypto_store.hpp"
#include "crypto/ed25519_provider.hpp"
#include "crypto/sr25519_provider.hpp"
#include "runtime/runtime_api/core.hpp"

#include <libp2p/host/host.hpp>
#include <libp2p/protocol/kademlia/kademlia.hpp>
#include <memory>

#include "log/logger.hpp"

namespace kagome::authority_discovery {

  class AddressPublisherImpl
      : public AddressPublisher,
        std::enable_shared_from_this<AddressPublisherImpl> {
   public:
    AddressPublisherImpl(std::shared_ptr<authority::AuthorityManager> authority_manager,
                         std::shared_ptr<blockchain::BlockTree> block_tree,
                         std::shared_ptr<crypto::SessionKeys> keys,
                         std::shared_ptr<crypto::CryptoStore> store,
                         std::shared_ptr<crypto::Ed25519Provider> crypto_provider,
                         std::shared_ptr<crypto::Sr25519Provider> crypto_provider2,
                         libp2p::Host &host,
                         std::shared_ptr<libp2p::protocol::kademlia::Kademlia> kademlia,
                         std::shared_ptr<libp2p::basic::Scheduler> scheduler);

    void run() override;

   private:
    outcome::result<void> publishOwnAddress();

    std::shared_ptr<authority::AuthorityManager> authority_manager_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;

    std::shared_ptr<crypto::SessionKeys> keys_;
    std::shared_ptr<crypto::CryptoStore> store_;

    std::shared_ptr<crypto::Ed25519Provider> crypto_provider_;
    std::shared_ptr<crypto::Sr25519Provider> crypto_provider2_;

    libp2p::Host &host_;
    std::shared_ptr<libp2p::protocol::kademlia::Kademlia> kademlia_;

    std::shared_ptr<libp2p::basic::Scheduler> scheduler_;

    log::Logger log_;
  };

}  // namespace kagome::authority_discovery

#endif  // KAGOME_ADDRESS_PUBLISHER_IMPL_HPP
