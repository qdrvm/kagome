/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/peer_client_libp2p.hpp"

#include "common/buffer.hpp"
#include "libp2p/basic/message_read_writer.hpp"
#include "network/impl/common.hpp"
#include "scale/scale.hpp"

namespace kagome::network {
  using common::Buffer;

  PeerClientLibp2p::PeerClientLibp2p(
      std::shared_ptr<NetworkState> network_state,
      libp2p::Host &host,
      libp2p::peer::PeerInfo peer_info,
      common::Logger logger)
      : network_state_{std::move(network_state)},
        host_{host},
        peer_info_{std::move(peer_info)},
        log_{std::move(logger)} {}

  void PeerClientLibp2p::blocksRequest(BlockRequest request,
                                       BlockResponseHandler handler) const {
    // request-response model requires us to open "sync" channel
    host_.newStream(
        peer_info_,
        kSyncProtocol,
        [self{shared_from_this()},
         request{std::move(request)},
         cb{std::move(handler)}](auto &&stream_res) mutable {
          if (!stream_res) {
            self->log_->error("cannot open a stream to peer {}",
                              self->peer_info_.id.toBase58());
            return cb(stream_res.error());
          }
          auto stream = std::move(stream_res.value());

          auto encoded_request_res = scale::encode(request);
          if (!encoded_request_res) {
            self->log_->error(
                "cannot open a encode a block request message: {}",
                encoded_request_res.error().message());
            return cb(encoded_request_res.error());
          }
          auto encoded_request =
              std::make_shared<Buffer>(std::move(encoded_request_res.value()));

          auto read_writer = std::make_shared<libp2p::basic::MessageReadWriter>(
              std::move(stream));
          read_writer->write(
              *encoded_request,
              [self{std::move(self)},
               encoded_request,
               read_writer,
               cb{std::move(cb)}](auto write_res) mutable {
                self->onBlocksRequestWritten(
                    std::move(write_res), read_writer, std::move(cb));
              });
        });
  }

  void PeerClientLibp2p::onBlocksRequestWritten(
      outcome::result<size_t> write_res,
      const std::shared_ptr<libp2p::basic::MessageReadWriter> &read_writer,
      BlockResponseHandler cb) const {
    if (!write_res) {
      log_->error("cannot write block request to stream");
      return cb(write_res.error());
    }

    read_writer->read(
        [self{shared_from_this()}, cb{std::move(cb)}](auto read_res) {
          if (!read_res) {
            self->log_->error("cannot read a block response message: {}",
                              read_res.error().message());
            return cb(read_res.error());
          }

          auto decoded_response_res =
              scale::decode<BlockResponse>(*read_res.value());
          if (!decoded_response_res) {
            self->log_->error("cannot decode a block response message: {}",
                              decoded_response_res.error().message());
            return cb(decoded_response_res.error());
          }

          return cb(std::move(decoded_response_res.value()));
        });
  }

  void PeerClientLibp2p::blockAnnounce(
      BlockAnnounce block_announce,
      std::function<void(const outcome::result<void> &)> handler) const {
    host_.newStream(
        peer_info_,
        kGossipProtocol,
        [self{shared_from_this()},
         announce{std::move(block_announce)},
         cb{std::move(handler)}](auto &&stream_res) mutable {
          if (!stream_res) {
            self->log_->error("cannot open a stream to peer {}",
                              self->peer_info_.id.toBase58());
            return cb(stream_res.error());
          }
          auto stream = std::move(stream_res.value());

          auto encoded_announce_res = scale::encode(announce);
          if (!encoded_announce_res) {
            self->log_->error(
                "cannot open a encode a block announce message: {}",
                encoded_announce_res.error().message());
            return cb(encoded_announce_res.error());
          }
          auto encoded_announce =
              std::make_shared<Buffer>(std::move(encoded_announce_res.value()));

          auto read_writer = std::make_shared<libp2p::basic::MessageReadWriter>(
              std::move(stream));
          read_writer->write(
              *encoded_announce,
              [self{std::move(self)}, cb{std::move(cb)}, encoded_announce](
                  auto write_res) {
                if (!write_res) {
                  self->log_->error("cannot write block announce to stream");
                  return cb(write_res.error());
                }
                cb(outcome::success());
              });
        });
  }
}  // namespace kagome::network
