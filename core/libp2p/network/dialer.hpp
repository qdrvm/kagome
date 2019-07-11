/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_DIALER_HPP
#define KAGOME_NETWORK_DIALER_HPP

#include "libp2p/connection/capable_connection.hpp"
#include "libp2p/peer/peer_info.hpp"
#include "libp2p/peer/protocol.hpp"

namespace libp2p::network {

  /**
   * @brief Class, which is capable of opening new connections and streams using
   * registered transports.
   */
  struct Dialer {
    virtual ~Dialer() = default;

    using DialResult =
        outcome::result<std::shared_ptr<connection::CapableConnection>>;
    using DialResultFunc = std::function<void(DialResult)>;

    using StreamResult = outcome::result<std::shared_ptr<connection::Stream>>;
    using StreamResultFunc = std::function<void(StreamResult)>;

    enum class Error {
      SUCCESS = 0,
      NO_KNOWN_ADDRESSES,
    };

    // Establishes a connection or returns existing one to a given peer
    virtual void dial(const peer::PeerInfo &p, DialResultFunc cb) = 0;

    // NewStream returns a new stream to given peer p.
    // If there is no connection to p, attempts to create one.
    virtual void newStream(const peer::PeerInfo &p,
                           const peer::Protocol &protocol,
                           StreamResultFunc cb) = 0;
  };

}  // namespace libp2p::network

OUTCOME_HPP_DECLARE_ERROR(libp2p::network, Dialer::Error);

#endif  // KAGOME_NETWORK_DIALER_HPP
