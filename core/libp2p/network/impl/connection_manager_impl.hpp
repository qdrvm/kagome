/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONNECTION_MANAGER_IMPL_HPP
#define KAGOME_CONNECTION_MANAGER_IMPL_HPP

#include "libp2p/network/connection_manager.hpp"
#include "libp2p/network/transport_manager.hpp"
#include "libp2p/peer/address_repository.hpp"
#include "libp2p/peer/peer_id.hpp"

namespace libp2p::network {

  class ConnectionManagerImpl : public ConnectionManager {
   public:
    ConnectionManagerImpl(
        std::shared_ptr<peer::AddressRepository> addrRepo,
        std::shared_ptr<network::TransportManager> transportManager);

    // get list of all connections (including inbound and outbound)
    std::vector<ConnectionSPtr> getConnections() const override;

    // get list of all outbound connections to a given peer.
    std::vector<ConnectionSPtr> getConnectionsToPeer(
        const peer::PeerId &p) const override;

    // get best connection to a given peer
    ConnectionSPtr getBestConnectionForPeer(
        const peer::PeerId &p) const override;

    // get connectedness information for given peer p
    Connectedness connectedness(const peer::PeerId &p) const override;

    // add connection to a given peer
    void addConnectionToPeer(const peer::PeerId &p, ConnectionSPtr c) override;

    void collectGarbage() override;

   private:
    std::shared_ptr<peer::AddressRepository> addr_repo_;
    std::shared_ptr<network::TransportManager> transport_manager_;

    std::unordered_map<peer::PeerId, std::vector<ConnectionSPtr>> connections_;
  };

}  // namespace libp2p::network

#endif  // KAGOME_CONNECTION_MANAGER_IMPL_HPP
