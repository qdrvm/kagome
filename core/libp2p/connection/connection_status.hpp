/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONNECTION_STATUS_HPP
#define KAGOME_CONNECTION_STATUS_HPP

#include <boost/variant.hpp>
#include "libp2p/error/error.hpp"

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
    using EventObject = boost::variant<error::Error, connection::Connection>;

    Status status;
    EventObject event_object;
  };
}  // namespace libp2p::connection

#endif  // KAGOME_CONNECTION_STATUS_HPP
