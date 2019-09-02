/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/gossiper_server_libp2p.hpp"

#include "libp2p/basic/message_read_writer.hpp"
#include "libp2p/connection/stream.hpp"
#include "network/impl/common.hpp"
#include "network/impl/scale_rpc_receiver.hpp"
#include "network/types/network_message.hpp"
#include "scale/scale.hpp"

namespace kagome::network {
  using libp2p::basic::MessageReadWriter;

  GossiperServerLibp2p::GossiperServerLibp2p(libp2p::Host &host,
                                             common::Logger log)
      : host_{host}, log_{std::move(log)} {}

  void GossiperServerLibp2p::start() {
    host_.setProtocolHandler(kGossipProtocol,
                             [self{shared_from_this()}](auto stream) {
                               self->handleGossipProtocol(std::move(stream));
                             });
  }

  void GossiperServerLibp2p::setBlockAnnounceHandler(
      BlockAnnounceHandler handler) {
    block_announce_handler_ = std::move(handler);
  }

  void GossiperServerLibp2p::handleGossipProtocol(
      std::shared_ptr<libp2p::connection::Stream> stream) const {
    ScaleRPCReceiver::receive<NetworkMessage>(
        std::make_shared<MessageReadWriter>(stream),
        [self{shared_from_this()}, stream](NetworkMessage msg) mutable {
          switch (msg.type) {
            case NetworkMessage::Type::BLOCK_ANNOUNCE: {
              auto announce_res = scale::decode<BlockAnnounce>(msg.body);
              if (!announce_res) {
                self->log_->error("cannot decode block announce: {}",
                                  announce_res.error().message());
                return stream->reset();
              }

              return self->handleBlockAnnounce(announce_res.value(),
                                               std::move(stream));
            }
            default:
              self->log_->error(
                  "unexpected message type arrived over the gossiper protocol");
              return stream->reset();
          }
        },
        [self{shared_from_this()}, stream](auto &&err) {
          self->log_->error("error while receiving block announce: {}",
                            err.error().message());
          stream->reset();
        });
  }

  void GossiperServerLibp2p::handleBlockAnnounce(
      const BlockAnnounce &announce,
      std::shared_ptr<libp2p::connection::Stream> stream) const {
    block_announce_handler_(announce);

    // read next message from the same stream
    handleGossipProtocol(std::move(stream));
  }
}  // namespace kagome::network
