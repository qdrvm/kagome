/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_SYNCPROTOCOLIMPL
#define KAGOME_NETWORK_SYNCPROTOCOLIMPL

#include "network/protocols/sync_protocol.hpp"

#include <memory>

#include <libp2p/connection/stream.hpp>
#include <libp2p/host/host.hpp>

#include "application/chain_spec.hpp"
#include "log/logger.hpp"
#include "network/sync_protocol_observer.hpp"

namespace kagome::network {

  using Stream = libp2p::connection::Stream;
  using Protocol = libp2p::peer::Protocol;
  using PeerId = libp2p::peer::PeerId;
  using PeerInfo = libp2p::peer::PeerInfo;

  class SyncProtocolImpl final
      : public SyncProtocol,
        public std::enable_shared_from_this<SyncProtocolImpl> {
   public:
    SyncProtocolImpl(libp2p::Host &host,
                 const application::ChainSpec &chain_spec,
                 std::shared_ptr<SyncProtocolObserver> sync_observer);

    const Protocol &protocol() const override {
      return protocol_;
    }

    bool start() override;
    bool stop() override;

    void onIncomingStream(std::shared_ptr<Stream> stream) override;
    void newOutgoingStream(
        const PeerInfo &peer_info,
        std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb)
        override;

    void request(const PeerId &peer_id,
                 BlocksRequest block_request,
                 std::function<void(outcome::result<BlocksResponse>)>
                     &&response_handler) override;

    void readRequest(std::shared_ptr<Stream> stream);

    void writeResponse(std::shared_ptr<Stream> stream,
                       const BlocksResponse &block_response);

    void writeRequest(std::shared_ptr<Stream> stream,
                      BlocksRequest block_request,
                      std::function<void(outcome::result<void>)> &&cb);

    void readResponse(std::shared_ptr<Stream> stream,
                      std::function<void(outcome::result<BlocksResponse>)>
                          &&response_handler);

   private:
    libp2p::Host &host_;
    std::shared_ptr<SyncProtocolObserver> sync_observer_;
    const libp2p::peer::Protocol protocol_;
    log::Logger log_ = log::createLogger("SyncProtocol", "kagome_protocols");
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_SYNCPROTOCOLIMPL
