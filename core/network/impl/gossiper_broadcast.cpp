/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/gossiper_broadcast.hpp"

#include <memory> 
#include <atomic>

#include "network/common.hpp"
#include "network/impl/loopback_stream.hpp"

namespace kagome::network {

  GossiperBroadcast::GossiperBroadcast(libp2p::Host &host)
      : logger_{common::createLogger("GossiperBroadcast")},
        stream_engine_{StreamEngine::create(host)} {}

  void GossiperBroadcast::reserveStream(
      const libp2p::peer::PeerInfo &peer_info,
      const libp2p::peer::Protocol &protocol,
      std::shared_ptr<libp2p::connection::Stream> stream) {
    stream_engine_->addReserved(peer_info,
                                protocol,
                                StreamEngine::ReservedStreamSetId::kRemote,
                                std::move(stream));
  }

  void GossiperBroadcast::reserveLoopbackStream(
      const libp2p::peer::PeerInfo &peer_info,
      const libp2p::peer::Protocol &protocol,
      std::shared_ptr<libp2p::connection::Stream> stream) {
    stream_engine_->addReserved(peer_info,
                                protocol,
                                StreamEngine::ReservedStreamSetId::kLoopback,
                                std::move(stream));
  }

  outcome::result<void> GossiperBroadcast::addStream(
      const libp2p::peer::Protocol &protocol,
      std::shared_ptr<libp2p::connection::Stream> stream) {
    return stream_engine_->add(protocol, std::move(stream));
  }

  uint32_t GossiperBroadcast::getActiveStreamNumber() {
    return stream_engine_->count();
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

  void GossiperBroadcast::send(const libp2p::peer::PeerId &peer_id,
                               const libp2p::peer::Protocol &protocol,
                               GossipMessage &&msg) {
    stream_engine_->send(StreamEngine::PeerInfo{.id = peer_id, .addresses = {}},
                         protocol,
                         std::move(msg));
  }

  void GossiperBroadcast::broadcast(const libp2p::peer::Protocol &protocol,
                                    GossipMessage &&msg) {
    stream_engine_->broadcast(protocol, std::move(msg));
    /*auto msg_send_lambda = [msg, this](auto stream) {
      auto s = stream;
      auto read_writer =
          std::make_shared<ScaleMessageReadWriter>(std::move(stream));
      read_writer->write(msg, [this, s](auto &&res) {
        if (not res) {
          logger_->error("Could not send message to {}: {}",
                         s->remotePeerId().value().toBase58(),
                         res.error().message());
        } else {
          logger_->debug("Message sent to {}",
                         s->remotePeerId().value().toBase58());
        }
      });
    };*/

    // iterate over the existing streams and send them the msg. If stream is
    // closed it is removed
    /*  auto stream_it = syncing_streams_.begin();
      while (stream_it != syncing_streams_.end()) {
        auto &[peerInfo, stream] = *stream_it;
        if (stream && !stream->isClosed()) {
          msg_send_lambda(stream);
          stream_it++;
        } else {
          // remove this stream
          stream_it = syncing_streams_.erase(stream_it);
        }
      }*/
    /*    for (const auto &[peerInfo, stream] : reserved_streams_) {
          if (stream && !stream->isClosed()) {
            msg_send_lambda(stream);
            continue;
          }
          // if stream does not exist or expired, open a new one
          host_.newStream(
              peerInfo,
              kGossipProtocol,
              [self{shared_from_this()}, info = peerInfo, msg_send_lambda](
                  auto &&stream_res) mutable {
                if (!stream_res) {
                  // we will try to open the stream again, when
                  // another gossip message arrives later
                  self->logger_->error(
                      "Could not send message to {} over new stream: {}",
                      info.id.toBase58(),
                      stream_res.error().message());
                  return;
                }

                // save the stream and send the message
                //self->reserved_streams_[info] = stream_res.value();
                msg_send_lambda(std::move(stream_res.value()));
              });
        }*/
  }
}  // namespace kagome::network
