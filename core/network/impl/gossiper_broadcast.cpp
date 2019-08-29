/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/gossiper_broadcast.hpp"

#include "network/gossiper_client.hpp"

namespace kagome::network {
  GossiperBroadcast::GossiperBroadcast(
      std::shared_ptr<NetworkState> network_state, common::Logger log)
      : network_state_{std::move(network_state)}, log_{std::move(log)} {}

  void GossiperBroadcast::blockAnnounce(BlockAnnounce block_announce) const {
    for (const auto &client : network_state_->gossiper_clients) {
      client->blockAnnounce(
          block_announce, [self{shared_from_this()}, client](auto &&write_res) {
            if (!write_res) {
              self->log_->error("cannot write block announce: {}",
                                write_res.error().message());
            }
          });
    }
  }
}  // namespace kagome::network
