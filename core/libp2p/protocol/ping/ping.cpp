/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol/ping/ping.hpp"

#include "libp2p/protocol/ping/ping_client_session.hpp"
#include "libp2p/protocol/ping/ping_server_session.hpp"

namespace {}

namespace libp2p::protocol {
  Ping::Ping(Host &host) : host_{host} {}

  peer::Protocol Ping::getProtocolId() const {
    return std::string{kPingProto};
  }

  void Ping::handle(StreamResult res) {
    if (!res) {
      return;
    }
    auto session =
        std::make_shared<PingServerSession>(host_, std::move(res.value()));
    session->start();
  }

  void Ping::startPinging(std::shared_ptr<connection::Stream> stream) {
    auto session =
        std::make_shared<PingClientSession>(host_, std::move(stream));
    session->start();
  }
}  // namespace libp2p::protocol
