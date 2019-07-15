/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PING_IMPL_HPP
#define KAGOME_PING_IMPL_HPP

#include <memory>

#include <outcome/outcome.hpp>
#include "libp2p/connection/capable_connection.hpp"
#include "libp2p/host.hpp"
#include "libp2p/protocol/base_protocol.hpp"

namespace libp2p::crypto::random {
  class RandomGenerator;
}

namespace libp2p::protocol {
  class PingClientSession;

  /**
   * Ping protocol, which is used to understand, if the peers are alive; will
   * continuously send Ping messages to another peer until it's dead or session
   * is closed
   */
  class Ping : public BaseProtocol, public std::enable_shared_from_this<Ping> {
   public:
    /**
     * Create an instance of Ping protocol handler
     * @param host to be in the instance
     * @param rand_gen - generator, which is used to generate Ping bytes
     */
    Ping(Host &host, std::shared_ptr<crypto::random::RandomGenerator> rand_gen);

    peer::Protocol getProtocolId() const override;

    void handle(StreamResult res) override;

    /**
     * Start pinging the peer
     * @param conn to the peer we want to ping
     * @param cb to be called, when a ping session is started, or error happens
     */
    void startPinging(
        const std::shared_ptr<connection::CapableConnection> &conn,
        std::function<void(outcome::result<std::shared_ptr<PingClientSession>>)>
            cb);

   private:
    Host &host_;
    std::shared_ptr<crypto::random::RandomGenerator> rand_gen_;
  };
}  // namespace libp2p::protocol

#endif  // KAGOME_PING_IMPL_HPP
