/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_API_ALLOW_UNSAFE_HPP
#define KAGOME_API_ALLOW_UNSAFE_HPP

#include <boost/asio/ip/tcp.hpp>

#include "application/app_configuration.hpp"

namespace kagome::api {
  struct AllowUnsafe {
    AllowUnsafe(const application::AppConfiguration &config)
        : config{config.allowUnsafeRpc()} {}

    bool allow(const boost::asio::ip::tcp::endpoint &endpoint) const {
      return config ? *config : endpoint.address().is_loopback();
    }

    std::optional<bool> config;
  };
}  // namespace kagome::api

#endif  // KAGOME_API_ALLOW_UNSAFE_HPP
