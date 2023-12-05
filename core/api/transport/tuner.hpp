/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

#include <boost/asio/ip/tcp.hpp>
#include "api/transport/rpc_io_context.hpp"
#include "log/logger.hpp"

namespace kagome::api {

  constexpr uint16_t kDefaultPortTolerance = 10;

  using Acceptor = boost::asio::ip::tcp::acceptor;
  using Endpoint = boost::asio::ip::tcp::endpoint;

  std::unique_ptr<Acceptor> acceptOnFreePort(
      std::shared_ptr<boost::asio::io_context> context,
      Endpoint endpoint,
      uint16_t port_tolerance,
      const log::Logger &logger);

}  // namespace kagome::api
