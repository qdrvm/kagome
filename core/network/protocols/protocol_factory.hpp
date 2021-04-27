/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_PROTOCOLFACTORY
#define KAGOME_NETWORK_PROTOCOLFACTORY

#include "application/app_configuration.hpp"
#include "consensus/babe/babe.hpp"
#include "network/impl/stream_engine.hpp"
#include "network/protocols/block_announce_protocol.hpp"
#include "network/protocols/gossip_protocol.hpp"
#include "network/protocols/grandpa_protocol.hpp"
#include "network/protocols/propagate_transactions_protocol.hpp"
#include "network/protocols/sup_protocol.hpp"
#include "network/protocols/sync_protocol.hpp"

namespace kagome::network {

  class ProtocolFactory final {
   public:
    ProtocolFactory(libp2p::Host &host,
                    const application::AppConfiguration &app_config,
                    const application::ChainSpec &chain_spec,
                    const OwnPeerInfo &own_info,
                    std::shared_ptr<boost::asio::io_context> io_context,
                    std::shared_ptr<blockchain::BlockStorage> storage,
                    std::shared_ptr<crypto::Hasher> hasher,
                    std::shared_ptr<StreamEngine> stream_engine);

    void setBlockTree(
        const std::shared_ptr<blockchain::BlockTree> &block_tree) {
      block_tree_ = block_tree;
    }

    void setBabe(const std::shared_ptr<consensus::babe::Babe> &babe) {
      babe_ = babe;
    }

    void setGrandpaObserver(
        const std::shared_ptr<consensus::grandpa::GrandpaObserver>
            &grandpa_observer) {
      grandpa_observer_ = grandpa_observer;
    }

    void setExtrinsicObserver(
        const std::shared_ptr<ExtrinsicObserver> &extrinsic_observer) {
      extrinsic_observer_ = extrinsic_observer;
    }

    void setSyncObserver(
        const std::shared_ptr<SyncProtocolObserver> &sync_observer) {
      sync_observer_ = sync_observer;
    }

    void setPeerManager(const std::shared_ptr<PeerManager> &peer_manager) {
      peer_manager_ = peer_manager;
    }

    std::shared_ptr<BlockAnnounceProtocol> makeBlockAnnounceProtocol() const;

    std::shared_ptr<GossipProtocol> makeGossipProtocol() const;

    std::shared_ptr<GrandpaProtocol> makeGrandpaProtocol() const;

    std::shared_ptr<PropagateTransactionsProtocol>
    makePropagateTransactionsProtocol() const;

    std::shared_ptr<SupProtocol> makeSupProtocol() const;

    std::shared_ptr<SyncProtocol> makeSyncProtocol() const;

   private:
    libp2p::Host &host_;
    const application::AppConfiguration &app_config_;
    const application::ChainSpec &chain_spec_;
    const OwnPeerInfo &own_info_;
    std::shared_ptr<boost::asio::io_context> io_context_;
    std::shared_ptr<blockchain::BlockStorage> storage_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<StreamEngine> stream_engine_;

    std::weak_ptr<blockchain::BlockTree> block_tree_;
    std::weak_ptr<consensus::babe::Babe> babe_;
    std::weak_ptr<consensus::grandpa::GrandpaObserver> grandpa_observer_;
    std::weak_ptr<ExtrinsicObserver> extrinsic_observer_;
    std::weak_ptr<SyncProtocolObserver> sync_observer_;
    std::weak_ptr<PeerManager> peer_manager_;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_PROTOCOLFACTORY
