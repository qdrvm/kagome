/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_LIBP2P_HPP
#define KAGOME_LIBP2P_HPP

#include <memory>

#include <rxcpp/rx.hpp>
#include "common/result.hpp"
#include "libp2p/common/peer.hpp"
#include "libp2p/common/peer_info.hpp"
#include "libp2p/connection/connection.hpp"
#include "libp2p/connection/muxed_connection.hpp"
#include "libp2p/discovery/peer_discovery.hpp"
#include "libp2p/error/error.hpp"
#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/multi/multihash.hpp"
#include "libp2p/muxer/muxer.hpp"
#include "libp2p/routing/router.hpp"
#include "libp2p/store/record.hpp"
#include "libp2p/store/record_store.hpp"
#include "libp2p/swarm/swarm.hpp"
#include "libp2p/transport/transport.hpp"
#include "libp2p/transport/transport_listener.hpp"

namespace libp2p {
  /**
   * Provides a way to instantiate a libp2p instance via joining different
   * components to it and dial to the peer
   */
  class Libp2p {
    /**
     * Add a swarm component
     * @param swarm to be added
     */
    virtual void addSwarm(std::unique_ptr<swarm::Swarm> swarm) = 0;

    /**
     * Add a routing component
     * @param routing to be added
     */
    virtual void addPeerRouting(std::unique_ptr<routing::Router> router) = 0;

    /**
     * Add a record store component
     * @param record store to be added
     */
    virtual void addRecordStore(std::unique_ptr<store::RecordStore> store) = 0;

    using DialResult =
        kagome::expected::Result<connection::Connection, error::Error>;

    /**
     * Dial to the peer via its info
     * @param peer_info of the peer
     * @return connection or error message
     */
    virtual DialResult dial(const common::PeerInfo &peer_info) = 0;

    /**
     * Dial to the peer via its id
     * @param peer_id of the peer
     * @return connection or error message
     */
    virtual DialResult dial(const multi::Multihash &peer_id) = 0;

    /**
     * Dial to the peer via its address
     * @param peer_address of the peer
     * @return connection or error message
     */
    virtual DialResult dial(const multi::Multiaddress &peer_address) = 0;
  };
}  // namespace libp2p

#endif  // KAGOME_LIBP2P_HPP
