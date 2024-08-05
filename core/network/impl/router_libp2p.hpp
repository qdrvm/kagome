/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "network/router.hpp"

#include "injector/lazy.hpp"
#include "log/logger.hpp"

namespace libp2p {
  struct Host;
}

namespace libp2p::multi {
  class Multiaddress;
}

namespace kagome {
  class PoolHandler;
}  // namespace kagome

namespace kagome::application {
  class AppConfiguration;
  class AppStateManager;
  class ChainSpec;
}  // namespace kagome::application

namespace kagome::blockchain {
  class BlockStorage;
}

namespace kagome::common {
  class MainThreadPool;
}

namespace kagome::network {
  struct OwnPeerInfo;
  struct BootstrapNodes;
  class WarpProtocol;
  class BeefyJustificationProtocol;
  class LightProtocol;
}  // namespace kagome::network

namespace kagome::network {
  class RouterLibp2p : public Router,
                       public std::enable_shared_from_this<RouterLibp2p> {
   public:
    RouterLibp2p(
        std::shared_ptr<application::AppStateManager> app_state_manager,
        common::MainThreadPool &main_thread_pool,
        libp2p::Host &host,
        const application::AppConfiguration &app_config,
        const OwnPeerInfo &own_info,
        const BootstrapNodes &bootstrap_nodes,
        LazySPtr<BlockAnnounceProtocol> block_announce_protocol,
        LazySPtr<GrandpaProtocol> grandpa_protocol,
        LazySPtr<SyncProtocol> sync_protocol,
        LazySPtr<StateProtocol> state_protocol,
        LazySPtr<WarpProtocol> warp_protocol,
        LazySPtr<BeefyProtocol> beefy_protocol,
        LazySPtr<BeefyJustificationProtocol> beefy_justifications_protocol,
        LazySPtr<LightProtocol> light_protocol,
        LazySPtr<PropagateTransactionsProtocol> propagate_transactions_protocol,
        LazySPtr<ValidationProtocol> validation_protocol,
        LazySPtr<CollationProtocol> collation_protocol,
        LazySPtr<CollationProtocolVStaging> collation_protocol_vstaging,
        LazySPtr<ValidationProtocolVStaging> validation_protocol_vstaging,
        LazySPtr<ReqCollationProtocol> req_collation_protocol,
        LazySPtr<ReqPovProtocol> req_pov_protocol,
        LazySPtr<FetchChunkProtocol> fetch_chunk_protocol,
        LazySPtr<FetchChunkProtocolObsolete> fetch_chunk_protocol_obsolete,
        LazySPtr<FetchAvailableDataProtocol> fetch_available_data_protocol,
        LazySPtr<StatementFetchingProtocol> statement_fetching_protocol,
        LazySPtr<SendDisputeProtocol> send_dispute_protocol,
        LazySPtr<libp2p::protocol::Ping> ping_protocol,
        LazySPtr<FetchAttestedCandidateProtocol> fetch_attested_candidate);

    /** @see AppStateManager::takeControl */
    void start();
    void startLibp2p();

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
    std::shared_ptr<CollationProtocolVStaging> getCollationProtocolVStaging()
        const override;
    std::shared_ptr<ValidationProtocol> getValidationProtocol() const override;
    std::shared_ptr<ValidationProtocolVStaging> getValidationProtocolVStaging()
        const override;
    std::shared_ptr<ReqCollationProtocol> getReqCollationProtocol()
        const override;
    std::shared_ptr<ReqPovProtocol> getReqPovProtocol() const override;
    std::shared_ptr<FetchChunkProtocol> getFetchChunkProtocol() const override;
    std::shared_ptr<FetchChunkProtocolObsolete> getFetchChunkProtocolObsolete()
        const override;
    std::shared_ptr<FetchAttestedCandidateProtocol>
    getFetchAttestedCandidateProtocol() const override;
    std::shared_ptr<FetchAvailableDataProtocol> getFetchAvailableDataProtocol()
        const override;
    std::shared_ptr<StatementFetchingProtocol> getFetchStatementProtocol()
        const override;

    std::shared_ptr<SendDisputeProtocol> getSendDisputeProtocol()
        const override;

    std::shared_ptr<libp2p::protocol::Ping> getPingProtocol() const override;

    std::shared_ptr<BeefyProtocol> getBeefyProtocol() const override;

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
    std::shared_ptr<PoolHandler> main_pool_handler_;

    LazySPtr<BlockAnnounceProtocol> block_announce_protocol_;
    LazySPtr<GrandpaProtocol> grandpa_protocol_;

    LazySPtr<SyncProtocol> sync_protocol_;
    LazySPtr<StateProtocol> state_protocol_;
    LazySPtr<WarpProtocol> warp_protocol_;
    LazySPtr<BeefyProtocol> beefy_protocol_;
    LazySPtr<BeefyJustificationProtocol> beefy_justifications_protocol_;
    LazySPtr<LightProtocol> light_protocol_;

    LazySPtr<PropagateTransactionsProtocol> propagate_transactions_protocol_;

    LazySPtr<ValidationProtocol> validation_protocol_;
    LazySPtr<CollationProtocol> collation_protocol_;
    LazySPtr<CollationProtocolVStaging> collation_protocol_vstaging_;
    LazySPtr<ValidationProtocolVStaging> validation_protocol_vstaging_;
    LazySPtr<ReqCollationProtocol> req_collation_protocol_;
    LazySPtr<ReqPovProtocol> req_pov_protocol_;
    LazySPtr<FetchChunkProtocol> fetch_chunk_protocol_;
    LazySPtr<FetchChunkProtocolObsolete> fetch_chunk_protocol_obsolete_;
    LazySPtr<FetchAvailableDataProtocol> fetch_available_data_protocol_;
    LazySPtr<StatementFetchingProtocol> statement_fetching_protocol_;

    LazySPtr<SendDisputeProtocol> send_dispute_protocol_;

    LazySPtr<libp2p::protocol::Ping> ping_protocol_;
    LazySPtr<FetchAttestedCandidateProtocol> fetch_attested_candidate_;

    log::Logger log_;
  };

}  // namespace kagome::network
