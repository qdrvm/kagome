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
  Ping::Ping(Host &host) : host_{host} {}

  peer::Protocol Ping::getProtocolId() const {
    return std::string{detail::kPingProto};
  }

  void Ping::handle(StreamResult res) {
    if (!res) {
      return;
    }
    auto session =
        std::make_shared<PingServerSession>(host_, std::move(res.value()));
    session->start();
  }

  std::unique_ptr<PingClientSession> Ping::startPinging(
      std::shared_ptr<connection::CapableConnection> conn) {
    auto peer_info =
    host_.newStream()

    auto session =
        std::make_unique<PingClientSession>(host_, std::move(stream));
    session->start();
    return session;
  }
}  // namespace libp2p::protocol
