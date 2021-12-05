/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/transport/tuner.hpp"

namespace kagome::api {

  std::unique_ptr<Acceptor> acceptOnFreePort(
      std::shared_ptr<boost::asio::io_context> context,
      Endpoint endpoint,
      uint16_t port_tolerance,
      const log::Logger &logger) {
    for (;;) {
      try {
        auto acceptor = std::make_unique<Acceptor>(*context, endpoint);
        return acceptor;
      } catch (
          const boost::wrapexcept<boost::system::system_error> &exception) {
        if ((exception.code() == boost::asio::error::address_in_use)
            and (port_tolerance > 0)) {
          SL_INFO(logger,
                  "Port {} is already in use, trying next one. ({} attempt(s) "
                  "left)",
                  endpoint.port(),
                  port_tolerance);
          --port_tolerance;
          endpoint.port(endpoint.port() + 1);
        } else {
          throw exception;
        }
      }
    }
  }

}  // namespace kagome::api