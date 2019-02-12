/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_LIBP2P_HPP
#define KAGOME_LIBP2P_HPP

#include <memory>

#include <rxcpp/rx.hpp>
#include "common/result.hpp"
#include "libp2p/common_objects/multiaddress.hpp"
#include "libp2p/common_objects/multihash.hpp"
#include "libp2p/common_objects/peer.hpp"
#include "libp2p/connection/connection.hpp"
#include "libp2p/connection/muxed_connection.hpp"
#include "libp2p/discovery/peer_discovery.hpp"
#include "libp2p/muxer/muxer.hpp"
#include "libp2p/routing/router.hpp"
#include "libp2p/store/record.hpp"
#include "libp2p/store/record_store.hpp"
#include "libp2p/swarm/dial_status.hpp"
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
    virtual void addSwarm(std::shared_ptr<swarm::Swarm> swarm) = 0;

    /**
     * Add a routing component
     * @param routing to be added
     */
    virtual void addPeerRouting(std::shared_ptr<routing::Router> router) = 0;

    /**
     * Add a record store component
     * @param record store to be added
     */
    virtual void addRecordStore(std::shared_ptr<store::RecordStore> store) = 0;

    using DialResult =
        kagome::expected::Result<connection::Connection, std::string>;

    /**
     * Dial to the peer via its info
     * @param peer_info of the peer
     * @return observable to connection or error message
     */
    virtual rxcpp::observable<DialResult> dial(
        const common::Peer::PeerInfo &peer_info) = 0;

    /**
     * Dial to the peer via its id
     * @param peer_id of the peer
     * @return observable to connection or error message
     */
    virtual rxcpp::observable<DialResult> dial(
        const common::Multihash &peer_id) = 0;

    /**
     * Dial to the peer via its address
     * @param peer_address of the peer
     * @return observable to connection or error message
     */
    virtual rxcpp::observable<DialResult> dial(
        const common::Multiaddress &peer_address) = 0;
  };
}  // namespace libp2p

// TODO: remove after PR is approved and clear the includes
/*
int foo() {
  // example of usage

  // Instantiate our libp2p node. Object of this class helps to establish
  // connection with other peers
  Libp2p node{priv_key, public_key};
  // swarm module's task is to establish connection with a known peer (it was
  // found via routing module) by choosing the most suitable transport for it
  node.addSwarm(std::make_shared<Swarm>());
  // finds peers; they are searched first locally, then through the network;
  // local database is updated automatically, when new peers are discovered
  node.addPeerRouting(std::make_shared<Router>());
  // all data, which is passed in the network, is records, which are saved in a
  // distributed record store; one can read about Kademlia DHT, for example
  node.addRecordStore(std::make_shared<RecordStore>());

  // now, we can start trying to establish connection with a peer of interest;
  // for instance, we know its address, but not id
  auto connection =
      node.dial(address);  // the id is going to be found by peer routing module
                           // and then the node will call underlying to swarm to
                           // establish connection

  // now, we have a connection to the other peer and can send some data through
  // it
  connection.write(stream_message);

  // and also we can read from it
  auto msg = connection.read();

  // one of the key features is optimal use of ports: several connection can be
  // multiplexed onto one port; firstly, create a multiplexor (muxer)
  Yamux muxer{};
  // then our connection can be attached to it, giving an object with the same
  // properties, but also with a couple of new features
  auto muxedConnection = muxer.attach(muxedConnection, isListener = false);
  // for example, now we can open a stream to that node
  stream = muxedConnection.newStream();
  // write and read to/from it
  stream.write(msg);
  auto msg = stream.read();
}
*/

#endif  // KAGOME_LIBP2P_HPP
