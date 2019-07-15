/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PING_HPP
#define KAGOME_PING_HPP

#include <memory>
#include <string_view>

#include "libp2p/connection/capable_connection.hpp"
#include "libp2p/host.hpp"
#include "libp2p/protocol/base_protocol.hpp"

namespace libp2p::protocol {
  class PingServerSession;
  class PingClientSession;

  /**
   * Ping protocol, which is used to understand, if the peers are alive; will
   * continuously send Ping messages to another peer until it's dead or session
   * is closed
   */
  class Ping : public BaseProtocol {
   public:
    /**
     * Create an instance of Ping protocol handler
     * @param host to be in the instance
     */
    explicit Ping(Host &host);

    peer::Protocol getProtocolId() const override;

    void handle(StreamResult res) override;

    /**
     * Start pinging the peer, to which the (\param conn) is
     * @return ping session, which can be closed; can be NULL in case of error
     */
    std::unique_ptr<PingClientSession> startPinging(
        std::shared_ptr<connection::CapableConnection> conn);

   private:
    Host &host_;
  };
}  // namespace libp2p::protocol

#endif  // KAGOME_PING_HPP
