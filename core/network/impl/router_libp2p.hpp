/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_IMPL_ROUTER_LIBP2P_HPP
#define KAGOME_NETWORK_IMPL_ROUTER_LIBP2P_HPP

#include "network/router.hpp"

#include "application/app_configuration.hpp"
#include "application/app_state_manager.hpp"
#include "libp2p/connection/loopback_stream.hpp"
#include "libp2p/host/host.hpp"
#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/protocol/ping.hpp"
#include "network/impl/protocols/protocol_factory.hpp"
#include "network/sync_protocol_observer.hpp"
#include "network/types/bootstrap_nodes.hpp"
#include "network/types/own_peer_info.hpp"

namespace kagome::application {
  class ChainSpec;
}

namespace kagome::blockchain {
  class BlockStorage;
}

namespace kagome::network {
  class RouterLibp2p : public Router,
                       public std::enable_shared_from_this<RouterLibp2p> {
   public:
    RouterLibp2p(
        std::shared_ptr<application::AppStateManager> app_state_manager,
        libp2p::Host &host,
        const application::AppConfiguration &app_config,
        const OwnPeerInfo &own_info,
        const BootstrapNodes &bootstrap_nodes,
        std::shared_ptr<libp2p::protocol::Ping> ping_proto,
        std::shared_ptr<network::ProtocolFactory> protocol_factory);

    ~RouterLibp2p() override = default;

    /** @see AppStateManager::takeControl */
    bool prepare();

    /** @see AppStateManager::takeControl */
    bool start();

    /** @see AppStateManager::takeControl */
    void stop();

    std::shared_ptr<BlockAnnounceProtocol> getBlockAnnounceProtocol()
        const override;
    std::shared_ptr<PropagateTransactionsProtocol>
    getPropagateTransactionsProtocol() const override;
    std::shared_ptr<StateProtocol> getStateProtocol() const override;
    std::shared_ptr<SyncProtocol> getSyncProtocol() const override;
    std::shared_ptr<GrandpaProtocol> getGrandpaProtocol() const override;
    std::shared_ptr<CollationProtocol> getCollationProtocol() const override;
    std::shared_ptr<ReqCollationProtocol> getReqCollationProtocol()
        const override;

    std::shared_ptr<libp2p::protocol::Ping> getPingProtocol() const override;

   private:
    /**
     * Appends /p2p/<peerid> part to ip4 and ip6 addresses which then passed to
     * host->listen method. Used further by Kademlia.
     * Non ip4 and ip6 addresses are left untouched
     * @param address multiaddress
     * @return an error if any
     */
    outcome::result<void> appendPeerIdToAddress(
        libp2p::multi::Multiaddress &address) const;

    std::shared_ptr<application::AppStateManager> app_state_manager_;
    libp2p::Host &host_;
    const application::AppConfiguration &app_config_;
    const OwnPeerInfo &own_info_;
    log::Logger log_;
    std::shared_ptr<libp2p::protocol::Ping> ping_protocol_;
    std::shared_ptr<network::ProtocolFactory> protocol_factory_;

    std::shared_ptr<BlockAnnounceProtocol> block_announce_protocol_;
    std::shared_ptr<GrandpaProtocol> grandpa_protocol_;
    std::shared_ptr<PropagateTransactionsProtocol>
        propagate_transaction_protocol_;
    std::shared_ptr<StateProtocol> state_protocol_;
    std::shared_ptr<SyncProtocol> sync_protocol_;
    std::shared_ptr<CollationProtocol> collation_protocol_;
    std::shared_ptr<ReqCollationProtocol> req_collation_protocol_;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_IMPL_ROUTER_LIBP2P_HPP
