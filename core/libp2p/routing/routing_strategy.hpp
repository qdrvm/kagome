/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ROUTING_STRATEGY_HPP
#define KAGOME_ROUTING_STRATEGY_HPP

namespace libp2p::routing {
  /**
   * Provides a way to find other peers in the network to establish
   * connections with them
   */
  struct RoutingAdaptor {
    virtual ~RoutingAdaptor() = default;
  };
}  // namespace libp2p::routing

#endif  // KAGOME_ROUTING_STRATEGY_HPP
