/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONNECTION_STATUS_HPP
#define KAGOME_CONNECTION_STATUS_HPP

#include <string>

#include <boost/variant.hpp>

namespace libp2p {
  namespace swarm {
    /**
     * Emitted, when something new happens with a connection
     */
    struct DialStatus {
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
  }  // namespace swarm
}  // namespace libp2p

#endif  // KAGOME_CONNECTION_STATUS_HPP
