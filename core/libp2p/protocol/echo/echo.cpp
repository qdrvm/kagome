/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "libp2p/protocol/echo/echo.hpp"

namespace libp2p::protocol {

  void Echo::handle(
      outcome::result<std::shared_ptr<connection::Stream>> rstream) {
    if (!rstream) {
      log_->info("incoming connection failed due to '{}'",
                 rstream.error().message());
      return;
    }

    auto session = std::make_shared<ServerEchoSession>(rstream.value(), config_);
    session->start();
  }

  peer::Protocol Echo::getProtocolId() const {
    return "/echo/1.0.0";
  }

  Echo::Echo(EchoConfig config) : config_(config) {}

  ClientEchoSession Echo::createClient(
      std::shared_ptr<connection::Stream> stream) {
    return ClientEchoSession{std::move(stream)};
  }

}  // namespace libp2p::protocol
