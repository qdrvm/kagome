/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "outcome/outcome.hpp"

#include <libp2p/connection/stream.hpp>
#include <libp2p/peer/peer_info.hpp>
#include <libp2p/peer/stream_protocols.hpp>

namespace kagome::network {

  using Stream = libp2p::connection::Stream;
  using PeerInfo = libp2p::peer::PeerInfo;
  using Protocol = libp2p::peer::ProtocolName;
  using PeerId = libp2p::peer::PeerId;
  using Protocols = libp2p::StreamProtocols;
  using ProtocolName = std::string;

  using namespace std::string_literals;

  class ProtocolBase {
   public:
    ProtocolBase() = default;
    ProtocolBase(ProtocolBase &&) = delete;
    ProtocolBase(const ProtocolBase &) = delete;
    virtual ~ProtocolBase() = default;
    ProtocolBase &operator=(ProtocolBase &&) = delete;
    ProtocolBase &operator=(const ProtocolBase &) = delete;

    virtual const ProtocolName &protocolName() const = 0;

    virtual bool start() = 0;

    virtual void onIncomingStream(std::shared_ptr<Stream> stream) = 0;
    virtual void newOutgoingStream(
        const PeerInfo &peer_info,
        std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb) = 0;
  };

}  // namespace kagome::network
