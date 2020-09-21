/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/gossiper_broadcast.hpp"

#include "network/common.hpp"
#include "network/helpers/scale_message_read_writer.hpp"
#include "network/impl/loopback_stream.hpp"

namespace kagome::network {

  GossiperBroadcast::GossiperBroadcast(libp2p::Host &host)
      : host_{host}, logger_{common::createLogger("GossiperBroadcast")} {}

  void GossiperBroadcast::reserveStream(
      const libp2p::peer::PeerInfo &peer_info,
      std::shared_ptr<libp2p::connection::Stream> stream) {
    streams_.emplace(peer_info, std::move(stream));
  }

  void GossiperBroadcast::transactionAnnounce(
      const TransactionAnnounce &announce) {
    logger_->debug("Gossip tx announce: {} extrinsics",
                   announce.extrinsics.size());
    GossipMessage message;
    message.type = GossipMessage::Type::TRANSACTIONS;
    message.data.put(scale::encode(announce).value());
    broadcast(std::move(message));
  }

  void GossiperBroadcast::blockAnnounce(const BlockAnnounce &announce) {
    logger_->debug("Gossip block announce: block number {}",
                   announce.header.number);
    GossipMessage message;
    message.type = GossipMessage::Type::BLOCK_ANNOUNCE;
    message.data.put(scale::encode(announce).value());
    broadcast(std::move(message));
  }

  void GossiperBroadcast::vote(
      const network::GrandpaVoteMessage &vote_message) {
    logger_->debug("Gossip vote message: grandpa round number {}",
                   vote_message.round_number);
    GossipMessage message;
    message.type = GossipMessage::Type::CONSENSUS;
    message.data.put(scale::encode(GrandpaMessage(vote_message)).value());

    broadcast(std::move(message));
  }

  void GossiperBroadcast::finalize(const network::GrandpaPreCommit &fin) {
    logger_->debug("Gossip fin message: grandpa round number {}",
                   fin.round_number);
    GossipMessage message;
    message.type = GossipMessage::Type::CONSENSUS;
    message.data.put(scale::encode(GrandpaMessage(fin)).value());

    broadcast(std::move(message));
  }

  void GossiperBroadcast::catchUpRequest(
      const libp2p::peer::PeerId &peer_id,
      const CatchUpRequest &catch_up_request) {
    logger_->debug("Gossip catch-up request: grandpa round number {}",
                   catch_up_request.round_number);
    GossipMessage message;
    message.type = GossipMessage::Type::CONSENSUS;
    message.data.put(scale::encode(GrandpaMessage(catch_up_request)).value());

    send(peer_id, std::move(message));
  }

  void GossiperBroadcast::catchUpResponse(
      const libp2p::peer::PeerId &peer_id,
      const CatchUpResponse &catch_up_response) {
    logger_->debug("Gossip catch-up response: grandpa round number {}",
                   catch_up_response.round_number);
    GossipMessage message;
    message.type = GossipMessage::Type::CONSENSUS;
    message.data.put(scale::encode(GrandpaMessage(catch_up_response)).value());

    send(peer_id, std::move(message));
  }

  void GossiperBroadcast::addStream(
      std::shared_ptr<libp2p::connection::Stream> stream) {
    auto peer_id_res = stream->remotePeerId();
    if (not peer_id_res.has_value()) {
      logger_->error("Can't get peer_id: {}", peer_id_res.error().message());
      return stream->reset();
    }
    auto &peer_id = peer_id_res.value();

    auto syncing_stream_it = syncing_streams_.find(peer_id);
    if (syncing_stream_it != syncing_streams_.end()) {
      if (syncing_stream_it->second != stream) {
        syncing_stream_it->second->reset();
        syncing_stream_it->second.swap(stream);
        logger_->debug("Syncing stream (peer_id={}) was replaced",
                       peer_id.toHex());
      }
      return;
    }

    logger_->debug("Syncing stream (peer_id={}) was emplaced", peer_id.toHex());
  }

  void GossiperBroadcast::send(const libp2p::peer::PeerId &peer_id,
                               GossipMessage &&msg) {
    auto msg_send_lambda = [msg, this](auto stream) {
      auto read_writer =
          std::make_shared<ScaleMessageReadWriter>(std::move(stream));
      read_writer->write(msg, [this](auto &&res) {
        if (not res) {
          logger_->error("Could not broadcast, reason: {}",
                         res.error().message());
        }
      });
    };

    // If stream exists then send them the msg
    // If stream is closed it is removed
    auto syncing_stream_it = syncing_streams_.find(peer_id);
    if (syncing_stream_it != syncing_streams_.end()) {
      auto &stream = syncing_stream_it->second;
      if (stream && !stream->isClosed()) {
        msg_send_lambda(stream);
      } else {
        // remove this stream
        syncing_streams_.erase(syncing_stream_it);
      }
      return;
    }

    auto stream_it = std::find_if(
        streams_.begin(), streams_.end(), [peer_id](const auto &item) {
          return item.first.id == peer_id;
        });
    if (stream_it != streams_.end()) {
      auto &[peerInfo, stream] = *stream_it;
      if (stream && !stream->isClosed()) {
        msg_send_lambda(stream);
      } else {
        // if stream does not exist or expired, open a new one
        host_.newStream(
            peerInfo,
            kGossipProtocol,
            [self{shared_from_this()}, info = peerInfo, msg_send_lambda](
                auto &&stream_res) mutable {
              if (!stream_res) {
                // we will try to open the stream again, when
                // another gossip message arrives later
                self->logger_->error("Could not send message to {} Error: {}",
                                     info.id.toBase58(),
                                     stream_res.error().message());
                return;
              }

              // save the stream and send the message
              self->streams_[info] = stream_res.value();
              msg_send_lambda(std::move(stream_res.value()));
            });
      }
      return;
    }
  }

  void GossiperBroadcast::broadcast(GossipMessage &&msg) {
    auto msg_send_lambda = [msg, this](auto stream) {
      auto s = stream;
      auto read_writer =
          std::make_shared<ScaleMessageReadWriter>(std::move(stream));
      read_writer->write(msg, [this, s](auto &&res) {
        if (not res) {
          logger_->error("Could not send message to {}: {}",
                         s->remotePeerId().value().toBase58(),
                         res.error().message());
        } else {
          logger_->error("Message sent to {}",
                         s->remotePeerId().value().toBase58());
        }
      });
    };

    // iterate over the existing streams and send them the msg. If stream is
    // closed it is removed
    auto stream_it = syncing_streams_.begin();
    while (stream_it != syncing_streams_.end()) {
      auto &[peerInfo, stream] = *stream_it;
      if (stream && !stream->isClosed()) {
        msg_send_lambda(stream);
        stream_it++;
      } else {
        // remove this stream
        stream_it = syncing_streams_.erase(stream_it);
      }
    }
    for (const auto &[peerInfo, stream] : streams_) {
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
            self->streams_[info] = stream_res.value();
            msg_send_lambda(std::move(stream_res.value()));
          });
    }
  }
}  // namespace kagome::network
