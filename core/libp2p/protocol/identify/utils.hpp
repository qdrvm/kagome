/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_ID_UTILS_HPP
#define KAGOME_ID_UTILS_HPP

#include <functional>
#include <memory>
#include <string>
#include <tuple>

#include <outcome/outcome.hpp>
#include "libp2p/connection/stream.hpp"
#include "libp2p/host.hpp"
#include "libp2p/multi/multiaddress.hpp"
#include "libp2p/network/connection_manager.hpp"
#include "libp2p/peer/protocol.hpp"

namespace libp2p::protocol::detail {
  /**
   * Get a tuple of stringified <PeerId, Multiaddress> of the peer the (\param
   * stream) is connected to
   */
  std::tuple<std::string, std::string> getPeerIdentity(
      const std::shared_ptr<libp2p::connection::Stream> &stream) {
    std::string id = "unknown";
    std::string addr = "unknown";
    if (auto id_res = stream->remotePeerId()) {
      id = id_res.value().toBase58();
    }
    if (auto addr_res = stream->remoteMultiaddr()) {
      addr = addr_res.value().getStringAddress();
    }
    return {std::move(id), std::move(addr)};
  }

  /**
   * Get collection of peers, to which we have at least one active connection
   * @param host of this node
   * @param conn_manager of this node
   * @return PeerInfo-s
   */
  std::vector<peer::PeerInfo> getActivePeers(
      Host &host, network::ConnectionManager &conn_manager) {
    std::vector<peer::PeerInfo> active_peers;
    std::unordered_set<peer::PeerId> active_peers_ids;

    for (const auto &conn : conn_manager.getConnections()) {
      active_peers_ids.insert(conn->remotePeer().value());
    }

    auto &peer_repo = host.getPeerRepository();
    active_peers.reserve(active_peers_ids.size());
    for (const auto &peer_id : active_peers_ids) {
      active_peers.push_back(peer_repo.getPeerInfo(peer_id));
    }

    return active_peers;
  }

  /**
   * Open a stream to each peer this host is connected to and execute a provided
   * function
   * @param host of this node
   * @param conn_manager of this node
   * @param protocol to open stream on
   * @param handler to be executed
   */
  void streamToEachConnectedPeer(
      Host &host, network::ConnectionManager &conn_manager,
      const peer::Protocol &protocol,
      const std::function<connection::Stream::Handler> &handler) {
    for (const auto &peer : getActivePeers(host, conn_manager)) {
      host.newStream(peer, protocol, handler);
    }
  }
}  // namespace libp2p::protocol::detail

#endif  // KAGOME_UTILS_HPP
