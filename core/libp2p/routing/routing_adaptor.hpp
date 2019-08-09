/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ROUTING_ADAPTOR_HPP
#define KAGOME_ROUTING_ADAPTOR_HPP

namespace libp2p::routing {
  /**
   * Provides a way to find other peers in the network to establish
   * connections with them
   */
  // A Peer Routing module offers a way for a libp2p Node to find the PeerInfo
  // of another Node, so that it can dial that node. In its most pure form, a
  // Peer Routing module should have an interface that takes a 'key', and
  // returns a set of PeerInfos
  struct RoutingAdaptor {
    virtual ~RoutingAdaptor() = default;
  };
}  // namespace libp2p::routing

#endif  // KAGOME_ROUTING_ADAPTOR_HPP
