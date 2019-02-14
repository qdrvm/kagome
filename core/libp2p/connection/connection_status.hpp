/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONNECTION_STATUS_HPP
#define KAGOME_CONNECTION_STATUS_HPP

#include <string>

#include <boost/variant.hpp>

namespace libp2p::connection {
  /**
   * Emitted, when something new happens with a connection
   */
  struct ConnectionStatus {
    enum class Status {
      kError,
      kMuxerUpgradeFailed,
      kConnectionAttemptFailed,
      kConnectionEstablished,
      kClosed
    };
    using EventObject = boost::variant<std::string, connection::Connection>;

    Status status;
    EventObject event_object;
  };
}  // namespace libp2p::connection

#endif  // KAGOME_CONNECTION_STATUS_HPP
