/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/gossiper_broadcast.hpp"

#include <boost/assert.hpp>
#include "network/common.hpp"
#include "network/helpers/scale_message_read_writer.hpp"

namespace kagome::network {

  GossiperBroadcast::GossiperBroadcast(libp2p::Host &host,
                                       const PeerList &peer_infos)
      : host_{host}, logger_{common::createLogger("GossiperBroadcast")} {
    BOOST_ASSERT(!peer_infos.peers.empty());

    streams_.reserve(peer_infos.peers.size());
    for (const auto &info : peer_infos.peers) {
      streams_.insert({info, nullptr});
    }
  }

  void GossiperBroadcast::blockAnnounce(const BlockAnnounce &announce) {
    logger_->info("Gossip block announce: block number {}",
                  announce.header.number);
    GossipMessage message;
    message.type = GossipMessage::Type::BLOCK_ANNOUNCE;
    message.data.put(scale::encode(announce).value());
    broadcast(std::move(message));
  }

  void GossiperBroadcast::vote(
      const consensus::grandpa::VoteMessage &vote_message) {
    logger_->info("Gossip vote message: grandpa round number {}",
                  vote_message.round_number);
    GossipMessage message;
    message.type = GossipMessage::Type::CONSENSUS;
    message.data.put(scale::encode(vote_message).value());

    broadcast(std::move(message));
  }

  void GossiperBroadcast::finalize(const consensus::grandpa::Fin &fin) {
    logger_->info("Gossip fin message: grandpa round number {}",
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
    auto msg_send_lambda = [msg](auto stream) {
      auto read_writer =
          std::make_shared<ScaleMessageReadWriter>(std::move(stream));
      read_writer->write(
          msg,
          [](auto &&) {  // we have nowhere to report the error to
          });
    };

    for (const auto &stream : syncing_streams_) {
      if (stream && !stream->isClosed()) {
        msg_send_lambda(stream);
      } else {
        // remove this stream
        syncing_streams_.erase(
            std::remove(
                syncing_streams_.begin(), syncing_streams_.end(), stream),
            syncing_streams_.end());
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
