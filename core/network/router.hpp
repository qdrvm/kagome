/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ROUTER_HPP
#define KAGOME_ROUTER_HPP

#include <libp2p/connection/stream.hpp>

namespace kagome::network {
  /**
   * Router, which reads and delivers different network messages to the
   * observers, responsible for their processing
   */
  class Router {
   protected:
    using Stream = libp2p::connection::Stream;

   public:
    virtual ~Router() = default;

    /**
     * Start accepting new connections and messages on this router
     */
    virtual void init() = 0;

    /**
     * Handle stream, which is opened over a Sync protocol
     * @param stream to be handled
     */
    virtual void handleSyncProtocol(
        const std::shared_ptr<Stream> &stream) const = 0;

    /**
     * Handle stream, which is opened over a Gossip protocol
     * @param stream to be handled
     */
    virtual void handleGossipProtocol(std::shared_ptr<Stream> stream) const = 0;
  };
}  // namespace kagome::network

#endif  // KAGOME_ROUTER_HPP
