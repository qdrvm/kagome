/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/gossiper_client_libp2p.hpp"

#include "common/buffer.hpp"
#include "network/impl/common.hpp"
#include "scale/scale.hpp"

namespace kagome::network {
  using common::Buffer;

  GossiperClientLibp2p::GossiperClientLibp2p(
      libp2p::peer::PeerInfo peer_info,
      std::shared_ptr<RPCSender<RPCInfoLibp2p>> rpc_sender,
      common::Logger log)
      : peer_info_{std::move(peer_info)},
        rpc_sender_{std::move(rpc_sender)},
        log_{std::move(log)} {}

  void GossiperClientLibp2p::blockAnnounce(
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
