/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/peer_client_libp2p.hpp"

#include "common/buffer.hpp"
#include "network/impl/common.hpp"
#include "scale/scale.hpp"

namespace kagome::network {
  using common::Buffer;

  PeerClientLibp2p::PeerClientLibp2p(
      libp2p::Host &host,
      libp2p::peer::PeerInfo peer_info,
      std::shared_ptr<RPCSender<RPCInfoLibp2p>> rpc_sender,
      common::Logger logger)
      : host_{host},
        peer_info_{std::move(peer_info)},
        rpc_sender_{std::move(rpc_sender)},
        log_{std::move(logger)} {}

  void PeerClientLibp2p::blocksRequest(BlocksRequest request,
                                       BlocksResponseHandler cb) const {
    auto encoded_request_res = scale::encode(request);
    if (!encoded_request_res) {
      log_->error("cannot encode a block request message: {}",
                  encoded_request_res.error().message());
      return cb(encoded_request_res.error());
    }

    // request-response model requires us to open "sync" channel
    rpc_sender_->sendWithResponse(
        {peer_info_, kSyncProtocol},
        std::make_shared<Buffer>(std::move(encoded_request_res.value())),
        [self{shared_from_this()},
         cb{std::move(cb)}](auto &&response_res) mutable {
          self->onBlocksResponseReceived(
              std::forward<decltype(response_res)>(response_res), cb);
        });
  }

  void PeerClientLibp2p::onBlocksResponseReceived(
      outcome::result<std::shared_ptr<common::Buffer>> encoded_response_res,
      const BlocksResponseHandler &cb) const {
    if (!encoded_response_res) {
      log_->error("cannot read a block response message: {}",
                  encoded_response_res.error().message());
      return cb(encoded_response_res.error());
    }

    auto decoded_response_res =
        scale::decode<BlocksResponse>(*encoded_response_res.value());
    if (!decoded_response_res) {
      log_->error("cannot decode a block response message: {}",
                  decoded_response_res.error().message());
      return cb(decoded_response_res.error());
    }

    return cb(std::move(decoded_response_res.value()));
  }

  void PeerClientLibp2p::blockAnnounce(
      BlockAnnounce block_announce,
      std::function<void(const outcome::result<void> &)> cb) const {
    auto encoded_announce_res = scale::encode(block_announce);
    if (!encoded_announce_res) {
      log_->error("cannot encode a block announce message: {}",
                  encoded_announce_res.error().message());
      return cb(encoded_announce_res.error());
    }

    rpc_sender_->sendWithoutResponse(
        {peer_info_, kGossipProtocol},
        std::make_shared<Buffer>(std::move(encoded_announce_res.value())),
        [self{shared_from_this()}, cb{std::move(cb)}](auto write_res) {
          if (!write_res) {
            self->log_->error("cannot write block announce to stream");
            return cb(write_res.error());
          }
          cb(outcome::success());
        });
  }
}  // namespace kagome::network
