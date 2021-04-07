/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/gossiper_broadcast.hpp"

#include <atomic>
#include <memory>

#include "application/chain_spec.hpp"
#include "network/common.hpp"
#include "network/impl/loopback_stream.hpp"

namespace kagome::network {
  KAGOME_DEFINE_CACHE(stream_engine);

  GossiperBroadcast::GossiperBroadcast(
      StreamEngine::StreamEnginePtr stream_engine,
      std::shared_ptr<primitives::events::ExtrinsicSubscriptionEngine>
          extrinsic_events_engine,
      std::shared_ptr<subscription::ExtrinsicEventKeyRepository>
          ext_event_key_repo,
      std::shared_ptr<kagome::application::ChainSpec> config,
      std::shared_ptr<BlockAnnounceProtocol> block_announce_protocol,
      std::shared_ptr<GossipProtocol> gossip_protocol,
      std::shared_ptr<PropagateTransactionsProtocol>
          propagate_transaction_protocol
//      ,
//      std::shared_ptr<SupProtocol> sup_protocol,
//      std::shared_ptr<SyncProtocol> sync_protocol
      )
      : logger_{log::createLogger("GossiperBroadcast", "network")},
        stream_engine_{std::move(stream_engine)},
        extrinsic_events_engine_{std::move(extrinsic_events_engine)},
        ext_event_key_repo_{std::move(ext_event_key_repo)},
        config_{std::move(config)},
        block_announce_protocol_{std::move(block_announce_protocol)},
        gossip_protocol_{std::move(gossip_protocol)},
        propagate_transaction_protocol_{
            std::move(propagate_transaction_protocol)}
//        ,
//        sup_protocol_{std::move(sup_protocol)},
//        sync_protocol_{std::move(sync_protocol)}
  {
    BOOST_ASSERT(stream_engine_ != nullptr);
    BOOST_ASSERT(extrinsic_events_engine_ != nullptr);
    BOOST_ASSERT(ext_event_key_repo_ != nullptr);
    BOOST_ASSERT(config_ != nullptr);
    BOOST_ASSERT(block_announce_protocol_ != nullptr);
    BOOST_ASSERT(gossip_protocol_ != nullptr);
    BOOST_ASSERT(propagate_transaction_protocol_ != nullptr);
//    BOOST_ASSERT(sup_protocol_ != nullptr);
//    BOOST_ASSERT(sync_protocol_ != nullptr);
  }

  void GossiperBroadcast::storeSelfPeerInfo(
      const libp2p::peer::PeerInfo &self_info) {
    self_info_ = self_info;
  }

  void GossiperBroadcast::propagateTransactions(
      gsl::span<const primitives::Transaction> txs) {
    logger_->debug("Propagate transactions : {} extrinsics", txs.size());

    std::vector<libp2p::peer::PeerId> peers;
    stream_engine_->forEachPeer(
        [&peers](const libp2p::peer::PeerId &peer_id, const auto &peer_map) {
          peers.push_back(peer_id);
        });
    if (peers.size() > 1) {
      for (const auto &tx : txs) {
        if (auto key = ext_event_key_repo_->getEventKey(tx); key.has_value()) {
          extrinsic_events_engine_->notify(
              key.value(),
              primitives::events::ExtrinsicLifecycleEvent::Broadcast(
                  key.value(), std::move(peers)));
        }
      }
    }

    PropagatedExtrinsics exts;
    exts.extrinsics.resize(txs.size());
    std::transform(
        txs.begin(), txs.end(), exts.extrinsics.begin(), [](auto &tx) {
          return tx.ext;
        });
    broadcast(propagate_transaction_protocol_, exts, NoData{});
  }

  void GossiperBroadcast::blockAnnounce(const BlockAnnounce &announce) {
    logger_->debug("Block announce: block number {}", announce.header.number);
    broadcast(block_announce_protocol_, announce, NoData{});
  }

  void GossiperBroadcast::vote(
      const network::GrandpaVoteMessage &vote_message) {
    logger_->debug("Gossip vote message: grandpa round number {}",
                   vote_message.round_number);
    GossipMessage message;
    message.type = GossipMessage::Type::CONSENSUS;
    message.data.put(scale::encode(GrandpaMessage(vote_message)).value());

    broadcast(gossip_protocol_, std::move(message));
  }

  void GossiperBroadcast::finalize(const network::GrandpaPreCommit &fin) {
    logger_->debug("Gossip fin message: grandpa round number {}",
                   fin.round_number);
    GossipMessage message;
    message.type = GossipMessage::Type::CONSENSUS;
    message.data.put(scale::encode(GrandpaMessage(fin)).value());

    broadcast(gossip_protocol_, std::move(message));
  }

  void GossiperBroadcast::catchUpRequest(
      const libp2p::peer::PeerId &peer_id,
      const CatchUpRequest &catch_up_request) {
    logger_->debug("Gossip catch-up request: grandpa round number {}",
                   catch_up_request.round_number);
    GossipMessage message;
    message.type = GossipMessage::Type::CONSENSUS;
    message.data.put(scale::encode(GrandpaMessage(catch_up_request)).value());

    send(peer_id, gossip_protocol_, std::move(message));
  }

  void GossiperBroadcast::catchUpResponse(
      const libp2p::peer::PeerId &peer_id,
      const CatchUpResponse &catch_up_response) {
    logger_->debug("Gossip catch-up response: grandpa round number {}",
                   catch_up_response.round_number);
    GossipMessage message;
    message.type = GossipMessage::Type::CONSENSUS;
    message.data.put(scale::encode(GrandpaMessage(catch_up_response)).value());

    send(peer_id, gossip_protocol_, std::move(message));
  }
}  // namespace kagome::network
