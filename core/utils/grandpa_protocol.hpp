/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_PEERING_UTILITY_GRANDPAPROTOCOL
#define KAGOME_PEERING_UTILITY_GRANDPAPROTOCOL

#include "network/protocol_base.hpp"

#include <memory>

#include <libp2p/connection/stream.hpp>
#include <libp2p/host/host.hpp>

#include "log/logger.hpp"
#include "network/impl/stream_engine.hpp"
#include "network/types/grandpa_message.hpp"

namespace kagome::network {

  using Stream = libp2p::connection::Stream;
  using Protocol = libp2p::peer::Protocol;
  using PeerId = libp2p::peer::PeerId;
  using PeerInfo = libp2p::peer::PeerInfo;

  class GrandpaProtocol final
      : public ProtocolBase,
        public std::enable_shared_from_this<GrandpaProtocol> {
   public:
    GrandpaProtocol() = delete;
    GrandpaProtocol(GrandpaProtocol &&) noexcept = delete;
    GrandpaProtocol(const GrandpaProtocol &) = delete;
    ~GrandpaProtocol() override = default;
    GrandpaProtocol &operator=(GrandpaProtocol &&) noexcept = delete;
    GrandpaProtocol &operator=(GrandpaProtocol const &) = delete;

    GrandpaProtocol(libp2p::Host &host,
                    std::shared_ptr<StreamEngine> stream_engine);

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

   private:
    enum class Direction { INCOMING, OUTGOING };
    void readHandshake(std::shared_ptr<Stream> stream,
                       Direction direction,
                       std::function<void(outcome::result<void>)> &&cb);

    void writeHandshake(std::shared_ptr<Stream> stream,
                        Direction direction,
                        std::function<void(outcome::result<void>)> &&cb);

    void read(std::shared_ptr<Stream> stream);

    void write(
        std::shared_ptr<Stream> stream,
        const int &msg,
        std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb);

    libp2p::Host &host_;
    std::shared_ptr<StreamEngine> stream_engine_;
    const libp2p::peer::Protocol protocol_;
    log::Logger log_ = log::createLogger("GrandpaProtocol", "kagome_protocols");
  };

}  // namespace kagome::network

#endif  // KAGOME_PEERING_UTILITY_GRANDPAROTOCOL
