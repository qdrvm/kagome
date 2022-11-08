/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_PROTOCOLFACTORY
#define KAGOME_NETWORK_PROTOCOLFACTORY

#include "application/app_configuration.hpp"
#include "consensus/babe/babe.hpp"
#include "network/impl/protocols/block_announce_protocol.hpp"
#include "network/impl/protocols/collation_protocol.hpp"
#include "network/impl/protocols/grandpa_protocol.hpp"
#include "network/impl/protocols/propagate_transactions_protocol.hpp"
#include "network/impl/protocols/protocol_req_collation.hpp"
#include "network/impl/protocols/request_response_protocol.hpp"
#include "network/impl/protocols/state_protocol_impl.hpp"
#include "network/impl/protocols/sync_protocol_impl.hpp"
#include "network/impl/stream_engine.hpp"
#include "network/reputation_repository.hpp"
#include "primitives/event_types.hpp"

#include <libp2p/basic/scheduler.hpp>

namespace kagome::network {

  class ProtocolFactory final {
   public:
    ProtocolFactory(
        libp2p::Host &host,
        const application::AppConfiguration &app_config,
        const application::ChainSpec &chain_spec,
        const OwnPeerInfo &own_info,
        std::shared_ptr<boost::asio::io_context> io_context,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<StreamEngine> stream_engine,
        std::shared_ptr<primitives::events::ExtrinsicSubscriptionEngine>
            extrinsic_events_engine,
        std::shared_ptr<subscription::ExtrinsicEventKeyRepository>
            ext_event_key_repo,
        std::shared_ptr<libp2p::basic::Scheduler> scheduler,
        std::shared_ptr<network::PeerView> peer_view,
        std::shared_ptr<ReputationRepository> reputation_repository);

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

    void setCollactionObserver(
        std::shared_ptr<CollationObserver> const &collation_observer) {
      collation_observer_ = collation_observer;
    }

    void setValidationObserver(
        std::shared_ptr<ValidationObserver> const &validation_observer) {
      validation_observer_ = validation_observer;
    }

    void setStateObserver(
        const std::shared_ptr<StateProtocolObserver> &state_observer) {
      state_observer_ = state_observer;
    }

    void setReqCollationObserver(
        std::shared_ptr<ReqCollationObserver> const &req_collation_observer) {
      req_collation_observer_ = req_collation_observer;
    }

    void setSyncObserver(
        const std::shared_ptr<SyncProtocolObserver> &sync_observer) {
      sync_observer_ = sync_observer;
    }

    void setPeerManager(const std::shared_ptr<PeerManager> &peer_manager) {
      peer_manager_ = peer_manager;
    }

    std::shared_ptr<BlockAnnounceProtocol> makeBlockAnnounceProtocol() const;

    std::shared_ptr<GrandpaProtocol> makeGrandpaProtocol() const;

    std::shared_ptr<PropagateTransactionsProtocol>
    makePropagateTransactionsProtocol() const;

    std::shared_ptr<StateProtocol> makeStateProtocol() const;
    std::shared_ptr<SyncProtocol> makeSyncProtocol() const;

    std::shared_ptr<CollationProtocol> makeCollationProtocol() const;
    std::shared_ptr<ValidationProtocol> makeValidationProtocol() const;
    std::shared_ptr<ReqCollationProtocol> makeReqCollationProtocol() const;

   private:
    libp2p::Host &host_;
    const application::AppConfiguration &app_config_;
    const application::ChainSpec &chain_spec_;
    const OwnPeerInfo &own_info_;
    std::shared_ptr<boost::asio::io_context> io_context_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<StreamEngine> stream_engine_;
    std::shared_ptr<primitives::events::ExtrinsicSubscriptionEngine>
        extrinsic_events_engine_;
    std::shared_ptr<subscription::ExtrinsicEventKeyRepository>
        ext_event_key_repo_;
    std::shared_ptr<ReputationRepository> reputation_repository_;
    std::shared_ptr<libp2p::basic::Scheduler> scheduler_;
    std::shared_ptr<network::PeerView> peer_view_;

    std::weak_ptr<blockchain::BlockTree> block_tree_;
    std::weak_ptr<consensus::babe::Babe> babe_;
    std::weak_ptr<consensus::grandpa::GrandpaObserver> grandpa_observer_;
    std::weak_ptr<ExtrinsicObserver> extrinsic_observer_;
    std::weak_ptr<StateProtocolObserver> state_observer_;
    std::weak_ptr<SyncProtocolObserver> sync_observer_;
    std::weak_ptr<PeerManager> peer_manager_;
    std::weak_ptr<CollationObserver> collation_observer_;
    std::weak_ptr<ValidationObserver> validation_observer_;
    std::weak_ptr<ReqCollationObserver> req_collation_observer_;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_PROTOCOLFACTORY
