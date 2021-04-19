/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_PROPAGATETRANSACTIONSPROTOCOL
#define KAGOME_NETWORK_PROPAGATETRANSACTIONSPROTOCOL

#include "network/protocol_base.hpp"

#include <memory>

#include <libp2p/connection/stream.hpp>
#include <libp2p/host/host.hpp>

#include "application/chain_spec.hpp"
#include "consensus/babe/babe.hpp"
#include "log/logger.hpp"
#include "network/extrinsic_observer.hpp"
#include "network/impl/stream_engine.hpp"
#include "network/types/propagate_transactions.hpp"

namespace kagome::network {

  using Stream = libp2p::connection::Stream;
  using Protocol = libp2p::peer::Protocol;
  using PeerId = libp2p::peer::PeerId;
  using PeerInfo = libp2p::peer::PeerInfo;

  class PropagateTransactionsProtocol final
      : public ProtocolBase,
        public std::enable_shared_from_this<PropagateTransactionsProtocol> {
   public:
    PropagateTransactionsProtocol() = delete;
    PropagateTransactionsProtocol(PropagateTransactionsProtocol &&) noexcept =
        delete;
    PropagateTransactionsProtocol(const PropagateTransactionsProtocol &) =
        delete;
    ~PropagateTransactionsProtocol() override = default;
    PropagateTransactionsProtocol &operator=(
        PropagateTransactionsProtocol &&) noexcept = delete;
    PropagateTransactionsProtocol &operator=(
        PropagateTransactionsProtocol const &) = delete;

    PropagateTransactionsProtocol(
        libp2p::Host &host,
        const application::ChainSpec &chain_spec,
        std::shared_ptr<consensus::babe::Babe> babe,
        std::shared_ptr<ExtrinsicObserver> extrinsic_observer,
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
    void readHandshake(
        std::shared_ptr<Stream> stream,
        Direction direction,
        std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb);

    void writeHandshake(
        std::shared_ptr<Stream> stream,
        Direction direction,
        std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb);

    void readPropagatedExtrinsics(std::shared_ptr<Stream> stream);

    void writePropagatedExtrinsics(
        std::shared_ptr<Stream> stream,
        const PropagatedExtrinsics &msg,
        std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb);

    libp2p::Host &host_;
    std::shared_ptr<consensus::babe::Babe> babe_;
    std::shared_ptr<ExtrinsicObserver> extrinsic_observer_;
    std::shared_ptr<StreamEngine> stream_engine_;
    const libp2p::peer::Protocol protocol_;
    log::Logger log_ = log::createLogger("PropagateTransactionsProtocol");
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_PROPAGATETRANSACTIONSPROTOCOL
