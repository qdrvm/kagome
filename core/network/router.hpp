/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ROUTER_HPP
#define KAGOME_ROUTER_HPP

#include <memory>

namespace kagome::network {
  /**
   * Router, which reads and delivers different network messages to the
   * observers, responsible for their processing
   * @tparam ChannelType - type of the channel, over which the messages are
   * read; can be, for example, a Libp2p Stream
   */
  template <typename ChannelType>
  struct Router {
    virtual ~Router() = default;

    /**
     * Handle channel, which is opened over a Sync protocol
     * @param channel to be handled
     */
    virtual void handleSyncProtocol(
        std::shared_ptr<ChannelType> channel) const = 0;

    /**
     * Handle channel, which is opened over a Gossip protocol
     * @param channel to be handled
     */
    virtual void handleGossipProtocol(
        std::shared_ptr<ChannelType> channel) const = 0;
  };
}  // namespace kagome::network

#endif  // KAGOME_ROUTER_HPP
