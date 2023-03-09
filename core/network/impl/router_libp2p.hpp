/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_IMPL_ROUTER_LIBP2P_HPP
#define KAGOME_NETWORK_IMPL_ROUTER_LIBP2P_HPP

#include "network/router.hpp"

#include "application/app_configuration.hpp"
#include "application/app_state_manager.hpp"
#include "injector/lazy.hpp"
#include "libp2p/connection/loopback_stream.hpp"
#include "libp2p/host/host.hpp"
#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/protocol/ping.hpp"
#include "network/impl/protocols/light.hpp"
#include "network/sync_protocol_observer.hpp"
#include "network/types/bootstrap_nodes.hpp"
#include "network/types/own_peer_info.hpp"
#include "network/warp/protocol.hpp"

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
        LazySPtr<BlockAnnounceProtocol> block_announce_protocol,
        LazySPtr<GrandpaProtocol> grandpa_protocol,
        LazySPtr<SyncProtocol> sync_protocol,
        LazySPtr<StateProtocol> state_protocol,
        LazySPtr<WarpProtocol> warp_protocol,
        LazySPtr<LightProtocol> light_protocol,
        LazySPtr<PropagateTransactionsProtocol> propagate_transactions_protocol,
        LazySPtr<ValidationProtocol> validation_protocol,
        LazySPtr<CollationProtocol> collation_protocol,
        LazySPtr<ReqCollationProtocol> req_collation_protocol,
        LazySPtr<ReqPovProtocol> req_pov_protocol,
        LazySPtr<FetchChunkProtocol> fetch_chunk_protocol,
        LazySPtr<FetchAvailableDataProtocol> fetch_available_data_protocol,
        LazySPtr<StatmentFetchingProtocol> statement_fetching_protocol,
        LazySPtr<SendDisputeProtocol> send_dispute_protocol,
        LazySPtr<libp2p::protocol::Ping> ping_protocol);

    ~RouterLibp2p() override = default;

    /** @see AppStateManager::takeControl */
    bool prepare();

    /** @see AppStateManager::takeControl */
    bool start();

    /** @see AppStateManager::takeControl */
    void stop();

    std::shared_ptr<BlockAnnounceProtocol> getBlockAnnounceProtocol()
        const override;
    std::shared_ptr<GrandpaProtocol> getGrandpaProtocol() const override;

    std::shared_ptr<SyncProtocol> getSyncProtocol() const override;
    std::shared_ptr<StateProtocol> getStateProtocol() const override;

    std::shared_ptr<PropagateTransactionsProtocol>
    getPropagateTransactionsProtocol() const override;

    std::shared_ptr<CollationProtocol> getCollationProtocol() const override;
    std::shared_ptr<ValidationProtocol> getValidationProtocol() const override;
    std::shared_ptr<ReqCollationProtocol> getReqCollationProtocol()
        const override;
    std::shared_ptr<ReqPovProtocol> getReqPovProtocol() const override;
    std::shared_ptr<FetchChunkProtocol> getFetchChunkProtocol() const override;
    std::shared_ptr<FetchAvailableDataProtocol> getFetchAvailableDataProtocol()
        const override;
    std::shared_ptr<StatmentFetchingProtocol> getFetchStatementProtocol()
        const override;
    std::shared_ptr<SendDisputeProtocol> getSendDisputeProtocol()
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

    LazySPtr<BlockAnnounceProtocol> block_announce_protocol_;
    LazySPtr<GrandpaProtocol> grandpa_protocol_;

    LazySPtr<SyncProtocol> sync_protocol_;
    LazySPtr<StateProtocol> state_protocol_;
    LazySPtr<WarpProtocol> warp_protocol_;
    LazySPtr<LightProtocol> light_protocol_;

    LazySPtr<PropagateTransactionsProtocol> propagate_transactions_protocol_;

    LazySPtr<ValidationProtocol> validation_protocol_;
    LazySPtr<CollationProtocol> collation_protocol_;
    LazySPtr<ReqCollationProtocol> req_collation_protocol_;
    LazySPtr<ReqPovProtocol> req_pov_protocol_;
    LazySPtr<FetchChunkProtocol> fetch_chunk_protocol_;
    LazySPtr<FetchAvailableDataProtocol> fetch_available_data_protocol_;
    LazySPtr<StatmentFetchingProtocol> statement_fetching_protocol_;
    LazySPtr<SendDisputeProtocol> send_dispute_protocol_;

    LazySPtr<libp2p::protocol::Ping> ping_protocol_;

    log::Logger log_;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_IMPL_ROUTER_LIBP2P_HPP
