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
      const consensus::grandpa::VoteMessage &vote_message) {
    logger_->debug("Gossip vote message: grandpa round number {}",
                   vote_message.round_number);
    GossipMessage message;
    message.type = GossipMessage::Type::CONSENSUS;
    message.data.put(scale::encode(vote_message).value());

    broadcast(std::move(message));
  }

  void GossiperBroadcast::finalize(const consensus::grandpa::Fin &fin) {
    logger_->debug("Gossip fin message: grandpa round number {}",
                   fin.round_number);
    GossipMessage message;
    message.type = GossipMessage::Type::CONSENSUS;
    message.data.put(scale::encode(fin).value());

    broadcast(std::move(message));
  }

  void GossiperBroadcast::addStream(
      std::shared_ptr<libp2p::connection::Stream> stream) {
    syncing_streams_.push_back(stream);
  }

  void GossiperBroadcast::broadcast(GossipMessage &&msg) {
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

    // iterate over the existing streams and send them the msg. If stream is
    // closed it is removed
    auto stream_it = syncing_streams_.begin();
    while (stream_it != syncing_streams_.end()) {
      auto stream = *stream_it;
      if (stream && !stream->isClosed()) {
        msg_send_lambda(stream);
        stream_it++;
      } else {
        // remove this stream
        stream_it = syncing_streams_.erase(stream_it);
      }
    }
    for (const auto &[info, stream] : streams_) {
      if (stream && !stream->isClosed()) {
        msg_send_lambda(stream);
        continue;
      }
      // if stream does not exist or expired, open a new one
      host_.newStream(info,
                      kGossipProtocol,
                      [self{shared_from_this()}, info = info, msg_send_lambda](
                          auto &&stream_res) mutable {
                        if (!stream_res) {
                          // we will try to open the stream again, when
                          // another gossip message arrives later
                          self->logger_->error(
                              "Could not send message to {} Error: {}",
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
