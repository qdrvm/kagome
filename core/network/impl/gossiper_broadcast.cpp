/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/gossiper_broadcast.hpp"

#include <atomic>
#include <memory>

#include "application/configuration_storage.hpp"
#include "network/common.hpp"
#include "network/impl/loopback_stream.hpp"

namespace kagome::network {
  KAGOME_DEFINE_CACHE(stream_engine);

  GossiperBroadcast::GossiperBroadcast(
      StreamEngine::StreamEnginePtr stream_engine,
      std::shared_ptr<kagome::application::ConfigurationStorage> config)
      : logger_{common::createLogger("GossiperBroadcast")},
        stream_engine_{std::move(stream_engine)},
        config_{std::move(config)} {}

  void GossiperBroadcast::reserveStream(
      const libp2p::peer::PeerInfo &peer_info,
      const libp2p::peer::Protocol &protocol,
      std::shared_ptr<libp2p::connection::Stream> stream) {
    stream_engine_->addReserved(peer_info, protocol, std::move(stream));
  }

  void GossiperBroadcast::storeSelfPeerInfo(
      const libp2p::peer::PeerInfo &self_info) {
    self_info_ = self_info;
  }

  outcome::result<void> GossiperBroadcast::addStream(
      const libp2p::peer::Protocol &protocol,
      std::shared_ptr<libp2p::connection::Stream> stream) {
    return stream_engine_->add(protocol, std::move(stream));
  }

  uint32_t GossiperBroadcast::getActiveStreamNumber() {
    BOOST_ASSERT(self_info_);
    return stream_engine_->count([&](const StreamEngine::PeerInfo &peer) {
      return *self_info_ != peer;
    });
  }

  void GossiperBroadcast::transactionAnnounce(
      const TransactionAnnounce &announce) {
    logger_->debug("Gossip tx announce: {} extrinsics",
                   announce.extrinsics.size());
    GossipMessage message;
    message.type = GossipMessage::Type::TRANSACTIONS;
    message.data.put(scale::encode(announce).value());
    broadcast(kGossipProtocol, std::move(message));
  }

  void GossiperBroadcast::propagateTransactions(
      const network::PropagatedTransactions &txs) {
    logger_->debug("Propagate transactions : {} extrinsics",
                   txs.extrinsics.size());
    broadcast(fmt::format(kPropagateTransactionsProtocol.data(),
                          config_->protocolId()),
              txs,
              NoData{});
  }

  void GossiperBroadcast::blockAnnounce(const BlockAnnounce &announce) {
    logger_->debug("Gossip block announce: block number {}",
                   announce.header.number);
    GossipMessage message;
    message.type = GossipMessage::Type::BLOCK_ANNOUNCE;
    message.data.put(scale::encode(announce).value());
    broadcast(kGossipProtocol, std::move(message));
  }

  void GossiperBroadcast::vote(
      const network::GrandpaVoteMessage &vote_message) {
    logger_->debug("Gossip vote message: grandpa round number {}",
                   vote_message.round_number);
    GossipMessage message;
    message.type = GossipMessage::Type::CONSENSUS;
    message.data.put(scale::encode(GrandpaMessage(vote_message)).value());

    broadcast(kGossipProtocol, std::move(message));
  }

  void GossiperBroadcast::finalize(const network::GrandpaPreCommit &fin) {
    logger_->debug("Gossip fin message: grandpa round number {}",
                   fin.round_number);
    GossipMessage message;
    message.type = GossipMessage::Type::CONSENSUS;
    message.data.put(scale::encode(GrandpaMessage(fin)).value());

    broadcast(kGossipProtocol, std::move(message));
  }

  void GossiperBroadcast::catchUpRequest(
      const libp2p::peer::PeerId &peer_id,
      const CatchUpRequest &catch_up_request) {
    logger_->debug("Gossip catch-up request: grandpa round number {}",
                   catch_up_request.round_number);
    GossipMessage message;
    message.type = GossipMessage::Type::CONSENSUS;
    message.data.put(scale::encode(GrandpaMessage(catch_up_request)).value());

    send(peer_id, kGossipProtocol, std::move(message));
  }

  void GossiperBroadcast::catchUpResponse(
      const libp2p::peer::PeerId &peer_id,
      const CatchUpResponse &catch_up_response) {
    logger_->debug("Gossip catch-up response: grandpa round number {}",
                   catch_up_response.round_number);
    GossipMessage message;
    message.type = GossipMessage::Type::CONSENSUS;
    message.data.put(scale::encode(GrandpaMessage(catch_up_response)).value());

    send(peer_id, kGossipProtocol, std::move(message));
  }
}  // namespace kagome::network
