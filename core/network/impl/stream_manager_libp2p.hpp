/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STREAM_MANAGER_LIBP2P_HPP
#define KAGOME_STREAM_MANAGER_LIBP2P_HPP

#include <unordered_map>

#include "libp2p/connection/stream.hpp"
#include "libp2p/host/host.hpp"
#include "libp2p/peer/peer_id.hpp"
#include "libp2p/peer/protocol.hpp"
#include "network/stream_manager.hpp"

namespace kagome::network {
  class StreamManagerLibp2p
      : public StreamManager<libp2p::peer::PeerInfo,
                             libp2p::peer::Protocol,
                             libp2p::connection::Stream>,
        public std::enable_shared_from_this<StreamManagerLibp2p> {
    using PeerInfo = libp2p::peer::PeerInfo;
    using Protocol = libp2p::peer::Protocol;
    using Stream = libp2p::connection::Stream;

   public:
    explicit StreamManagerLibp2p(libp2p::Host &host);

    ~StreamManagerLibp2p() override = default;

    void submitStream(const PeerInfo &id,
                      const Protocol &protocol,
                      std::shared_ptr<Stream> stream) override;

    void getStream(const PeerInfo &id,
                   const Protocol &protocol,
                   std::function<void(outcome::result<std::shared_ptr<Stream>>)>
                       cb) override;

   private:
    void openStream(
        const PeerInfo &id,
        const Protocol &protocol,
        std::function<void(outcome::result<std::shared_ptr<Stream>>)> cb);

    libp2p::Host &host_;

    std::unordered_map<PeerInfo,
                       std::unordered_map<Protocol, std::shared_ptr<Stream>>>
        streams_{};
  };
}  // namespace kagome::network

#endif  // KAGOME_STREAM_MANAGER_LIBP2P_HPP
