/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_LISTENER_IMPL_HPP
#define KAGOME_LISTENER_IMPL_HPP

#include "libp2p/connection/capable_connection.hpp"
#include "libp2p/network/connection_manager.hpp"
#include "libp2p/network/listener.hpp"
#include "libp2p/network/transport_manager.hpp"
#include "libp2p/peer/address_repository.hpp"
#include "libp2p/protocol_muxer/protocol_muxer.hpp"

namespace libp2p::network {

  class ListenerImpl : public Listener {
   public:
    ~ListenerImpl() override = default;

    ListenerImpl(std::shared_ptr<peer::AddressRepository> addrrepo,
                 std::shared_ptr<protocol_muxer::ProtocolMuxer> multiselect,
                 std::shared_ptr<Router> router,
                 std::shared_ptr<TransportManager> tmgr,
                 std::shared_ptr<ConnectionManager> cmgr);

    bool isStarted() const override;

    // starts listening on all provided multiaddresses
    void start() override;

    // stops listening on all multiaddresses
    void stop() override;

    outcome::result<void> closeListener(const multi::Multiaddress &ma) override;

    // Listen tells the network to start listening on given multiaddr.
    // May be executed multiple times (on different addresses/protocols).
    outcome::result<void> listen(const multi::Multiaddress &ma) override;

    // get all addresses we are listening on. will be exactly as we supplied to
    // listen.
    std::vector<multi::Multiaddress> getListenAddresses() const override;

    // get all addresses we are listening on. may be different from those
    // supplied to `listen`. example: /ip4/0.0.0.0/tcp/0 ->
    // /ip4/127.0.0.1/tcp/30000 and /ip4/192.168.1.2/tcp/30000
    outcome::result<std::vector<multi::Multiaddress>>
    getListenAddressesInterfaces() const override;

    void handleProtocol(
        std::shared_ptr<protocol::BaseProtocol> protocol) override;

    void setProtocolHandler(const peer::Protocol &protocol,
                            StreamResultFunc cb) override;

    void setProtocolHandler(const peer::Protocol &protocol, StreamResultFunc cb,
                            Router::ProtoPredicate predicate) override;

   private:
    bool started = false;

    // clang-format off
    std::unordered_map<multi::Multiaddress, std::shared_ptr<transport::TransportListener>> listeners_;
    // clang-format on

    std::shared_ptr<peer::AddressRepository> addrrepo_;
    std::shared_ptr<protocol_muxer::ProtocolMuxer> multiselect_;
    std::shared_ptr<network::Router> router_;
    std::shared_ptr<TransportManager> tmgr_;
    std::shared_ptr<ConnectionManager> cmgr_;

    void onConnection(
        outcome::result<std::shared_ptr<connection::CapableConnection>>);
  };

}  // namespace libp2p::network

#endif  // KAGOME_LISTENER_IMPL_HPP
