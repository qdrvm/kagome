/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/gossiper_server_libp2p.hpp"

#include "libp2p/basic/message_read_writer.hpp"
#include "libp2p/connection/stream.hpp"
#include "network/impl/common.hpp"
#include "scale/scale.hpp"

namespace kagome::network {
  using libp2p::basic::MessageReadWriter;

  GossiperServerLibp2p::GossiperServerLibp2p(libp2p::Host &host,
                                             common::Logger log)
      : host_{host}, log_{std::move(log)} {}

  void GossiperServerLibp2p::start() {
    host_.setProtocolHandler(
        kGossipProtocol, [self{shared_from_this()}](auto stream) {
          self->handleGossipProtocol(
              std::make_shared<MessageReadWriter>(stream), std::move(stream));
        });
  }

  void GossiperServerLibp2p::setBlockAnnounceHandler(
      BlockAnnounceHandler handler) {
    block_announce_handler_ = std::move(handler);
  }

  void GossiperServerLibp2p::handleGossipProtocol(
      const std::shared_ptr<libp2p::basic::MessageReadWriter> &read_writer,
      std::shared_ptr<libp2p::connection::Stream> stream) const {
    read_writer->read([self{shared_from_this()},
                       read_writer,
                       stream = std::move(stream)](auto &&read_res) mutable {
      if (!read_res) {
        self->log_->error("cannot read message from the stream: {}",
                          read_res.error().message());
        return stream->reset();
      }

      auto announce_candidate = scale::decode<BlockAnnounce>(*read_res.value());
      if (announce_candidate) {
        return self->handleBlockAnnounce(
            announce_candidate.value(), read_writer, std::move(stream));
      }

      self->log_->error("some unknown message type was received");
      return stream->reset();
    });
  }

  void GossiperServerLibp2p::handleBlockAnnounce(
      const BlockAnnounce &announce,
      const std::shared_ptr<libp2p::basic::MessageReadWriter> &read_writer,
      std::shared_ptr<libp2p::connection::Stream> stream) const {
    block_announce_handler_(announce);

    // read next message from the same stream
    handleGossipProtocol(read_writer, std::move(stream));
  }
}  // namespace kagome::network
