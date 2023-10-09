/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio/ip/tcp.hpp>

#include "application/app_configuration.hpp"

namespace kagome::api {
  struct AllowUnsafe {
    using Config = application::AppConfiguration::AllowUnsafeRpc;
    AllowUnsafe(const application::AppConfiguration &config)
        : config{config.allowUnsafeRpc()} {}

    bool allow(const boost::asio::ip::tcp::endpoint &endpoint) const {
      if (config == Config::kAuto) {
        return endpoint.address().is_loopback();
      }
      return config == Config::kUnsafe;
    }

    Config config;
  };
}  // namespace kagome::api
