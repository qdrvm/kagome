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
    logger_->debug("Gossip block announce: block number {}",
                   announce.header.number);
    broadcast(announce);
  }

  void GossiperBroadcast::vote(const consensus::grandpa::VoteMessage &msg) {
    broadcast(msg);
  }

  void GossiperBroadcast::fin(const consensus::grandpa::Fin &msg) {
    broadcast(msg);
  }

  template <typename MsgType>
  void GossiperBroadcast::broadcast(MsgType &&msg) {
    auto msg_send_lambda = [msg](auto stream) {
      auto read_writer =
          std::make_shared<ScaleMessageReadWriter>(std::move(stream));
      read_writer->write(
          msg,
          [](auto &&) {  // we have nowhere to report the error to
          });
    };

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
                          return;
                        }

                        // save the stream and send the message
                        self->streams_[info] = stream_res.value();
                        msg_send_lambda(std::move(stream_res.value()));
                      });
    }
  }

}  // namespace kagome::network
