/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/peer_server_libp2p.hpp"

#include "libp2p/connection/stream.hpp"
#include "network/impl/common.hpp"
#include "scale/scale.hpp"

namespace kagome::network {
  using libp2p::basic::MessageReadWriter;

  PeerServerLibp2p::PeerServerLibp2p(libp2p::Host &host,
                                     libp2p::peer::PeerInfo peer_info,
                                     common::Logger logger)
      : host_{host},
        peer_info_{std::move(peer_info)},
        log_{std::move(logger)} {}

  void PeerServerLibp2p::start() {
    host_.setProtocolHandler(kSyncProtocol,
                             [self{shared_from_this()}](auto stream) mutable {
                               self->handleSyncProto(std::move(stream));
                             });
    host_.setProtocolHandler(
        kGossipProtocol, [self{shared_from_this()}](auto stream) mutable {
          self->handleGossipProto(std::make_shared<MessageReadWriter>(stream),
                                  stream);
        });
  }

  void PeerServerLibp2p::onBlocksRequest(BlocksRequestHandler handler) {
    blocks_request_handler_ = std::move(handler);
  }

  void PeerServerLibp2p::onBlockAnnounce(BlockAnnounceHandler handler) {
    block_announce_handler_ = std::move(handler);
  }

  void PeerServerLibp2p::handleSyncProto(
      std::shared_ptr<libp2p::connection::Stream> stream) const {
    auto read_writer = std::make_shared<MessageReadWriter>(stream);
    read_writer->read([self{shared_from_this()},
                       read_writer,
                       stream{std::move(stream)}](auto &&read_res) mutable {
      if (!read_res) {
        self->log_->error("cannot read message from the stream: {}",
                          read_res.error().message());
        return stream->reset();
      }

      // several types of messages can arrive over Sync protocol (at least, in
      // the probable future)
      auto blocks_request_candidate =
          scale::decode<BlocksRequest>(*read_res.value());
      if (blocks_request_candidate) {
        if (!self->handleBlocksRequest(blocks_request_candidate.value(),
                                       read_writer)) {
          stream->reset();
        }
        return;
      }

      self->log_->error("some unknown message type was received");
      return stream->reset();
    });
  }

  bool PeerServerLibp2p::handleBlocksRequest(
      const BlocksRequest &request,
      const std::shared_ptr<libp2p::basic::MessageReadWriter> &read_writer)
      const {
    auto response_res = blocks_request_handler_(request);
    if (!response_res) {
      log_->error("cannot process blocks request: {}",
                  response_res.error().message());
      return false;
    }

    auto encoded_response_res = scale::encode(std::move(response_res.value()));
    if (!encoded_response_res) {
      log_->error("cannot encode blocks response: {}",
                  encoded_response_res.error().message());
      return false;
    }
    auto response = std::make_shared<common::Buffer>(
        std::move(encoded_response_res.value()));

    read_writer->write(*response,
                       [self{shared_from_this()}, response](auto &&write_res) {
                         if (!write_res) {
                           self->log_->error("cannot write blocks response: {}",
                                             write_res.error().message());
                         }
                       });
    return true;
  }

  void PeerServerLibp2p::handleGossipProto(
      const std::shared_ptr<libp2p::basic::MessageReadWriter> &read_writer,
      std::shared_ptr<libp2p::connection::Stream> stream) const {
    read_writer->read([self{shared_from_this()},
                       read_writer,
                       stream = std::move(stream)](auto &&read_res) mutable {
      if (!read_res) {
        return self->log_->error("cannot read message from the stream: {}",
                                 read_res.error().message());
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

  void PeerServerLibp2p::handleBlockAnnounce(
      const BlockAnnounce &announce,
      const std::shared_ptr<libp2p::basic::MessageReadWriter> &read_writer,
      std::shared_ptr<libp2p::connection::Stream> stream) const {
    block_announce_handler_(announce);
    handleGossipProto(read_writer, std::move(stream));
  }
}  // namespace kagome::network
