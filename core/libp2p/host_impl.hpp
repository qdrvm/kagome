/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_HOST_IMPL_HPP
#define KAGOME_HOST_IMPL_HPP

#include "libp2p/config.hpp"
#include "libp2p/host.hpp"

namespace libp2p {

  class HostImpl : public Host {
   public:
    ~HostImpl() override = default;

    std::string_view getLibp2pVersion() const noexcept override;

    std::string_view getLibp2pClientVersion() const noexcept override;

    peer::PeerId getId() const noexcept override;

    peer::PeerInfo getPeerInfo() const noexcept override;

    gsl::span<const multi::Multiaddress> getListenAddresses() const override;

    void setProtocolHandler(
        const peer::Protocol &proto,
        const std::function<connection::Stream::Handler> &handler) override;

    void setProtocolHandler(
        const std::string &prefix,
        const std::function<connection::Stream::Handler> &handler,
        const std::function<bool(const peer::Protocol &)> &predicate) override;

    outcome::result<void> connect(const peer::PeerInfo &p) override;

    outcome::result<void> newStream(
        const peer::PeerInfo &p, const peer::Protocol &protocol,
        const std::function<connection::Stream::Handler> &handler) override;

    const network::Network &network() const noexcept override;

    peer::PeerRepository &peerRepository() const noexcept override;

    const network::Router &router() const noexcept override;

   private:
    friend class HostBuilder;

    HostImpl(Config config, peer::PeerId peer_id);

    Config config_;

    peer::PeerId id_;
    std::unique_ptr<network::Network> network_;
    std::unique_ptr<network::Router> router_;
  };

}  // namespace libp2p

#endif  // KAGOME_HOST_IMPL_HPP
