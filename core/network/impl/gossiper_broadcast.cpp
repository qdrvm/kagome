/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/gossiper_broadcast.hpp"

#include "network/common.hpp"
#include "network/helpers/scale_message_read_writer.hpp"

namespace kagome::network {

  GossiperBroadcast::GossiperBroadcast(
      std::shared_ptr<Libp2pStreamManager> stream_manager,
      std::unordered_set<libp2p::peer::PeerInfo> peer_infos)
      : stream_manager_{std::move(stream_manager)},
        peer_infos_{std::move(peer_infos)} {}

  void GossiperBroadcast::blockAnnounce(const BlockAnnounce &announce) const {
    broadcast(announce);
  }

  void GossiperBroadcast::precommit(Precommit pc) const {
    broadcast(pc);
  }

  void GossiperBroadcast::prevote(Prevote pv) const {
    broadcast(pv);
  }

  void GossiperBroadcast::primaryPropose(PrimaryPropose pv) const {
    broadcast(pv);
  }

  template <typename MsgType>
  void GossiperBroadcast::broadcast(MsgType &&msg) const {
    for (const auto &info : peer_infos_) {
      stream_manager_->getStream(
          info, kGossipProtocol, [msg](auto &&stream_res) {
            if (!stream_res) {
              return;
            }

            auto read_writer = std::make_shared<ScaleMessageReadWriter>(
                std::move(stream_res.value()));
            read_writer->write(
                msg,
                [](auto &&) {  // we have nowhere to report the error to
                });
          });
    }
  }

}  // namespace kagome::network
