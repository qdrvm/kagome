/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol/ping/ping.hpp"

#include "libp2p/protocol/ping/common.hpp"
#include "libp2p/protocol/ping/ping_client_session.hpp"
#include "libp2p/protocol/ping/ping_server_session.hpp"

namespace {}

namespace libp2p::protocol {
  Ping::Ping(Host &host,
             std::shared_ptr<crypto::random::RandomGenerator> rand_gen)
      : host_{host}, rand_gen_{std::move(rand_gen)} {}

  peer::Protocol Ping::getProtocolId() const {
    return detail::kPingProto;
  }

  void Ping::handle(StreamResult res) {
    if (!res) {
      return;
    }
    auto session = std::make_shared<PingServerSession>(std::move(res.value()));
    session->start();
  }

  void Ping::startPinging(
      const std::shared_ptr<connection::CapableConnection> &conn,
      std::function<void(outcome::result<std::shared_ptr<PingClientSession>>)>
          cb) {
    auto remote_peer = conn->remotePeer();
    if (!remote_peer) {
      return cb(remote_peer.error());
    }
    auto peer_info = host_.getPeerRepository().getPeerInfo(remote_peer.value());
    return host_.newStream(
        peer_info, detail::kPingProto,
        [self{shared_from_this()}, cb = std::move(cb)](auto &&stream_res) {
          if (!stream_res) {
            return cb(stream_res.error());
          }
          auto session = std::make_shared<PingClientSession>(
              std::move(stream_res.value()), self->rand_gen_);
          session->start();
          cb(std::move(session));
        });
  }
}  // namespace libp2p::protocol
