/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_SUPPROTOCOL
#define KAGOME_NETWORK_SUPPROTOCOL

#include <memory>
#include "network/protocol_base.hpp"

#include <libp2p/connection/stream.hpp>
#include <libp2p/host/host.hpp>

#include "blockchain/block_storage.hpp"
#include "blockchain/block_tree.hpp"
#include "log/logger.hpp"
#include "network/impl/stream_engine.hpp"
#include "network/types/status.hpp"
#include "outcome/outcome.hpp"

namespace kagome::network {

  using Stream = libp2p::connection::Stream;
  using Protocol = libp2p::peer::Protocol;
  using PeerId = libp2p::peer::PeerId;
  using PeerInfo = libp2p::peer::PeerInfo;

  class SupProtocol final : public ProtocolBase,
                            public std::enable_shared_from_this<SupProtocol> {
   public:
    enum class Error { GONE = 1, CAN_NOT_CREATE_STATUS };

    SupProtocol() = delete;
    SupProtocol(SupProtocol &&) noexcept = delete;
    SupProtocol(const SupProtocol &) = delete;
    ~SupProtocol() override = default;
    SupProtocol &operator=(SupProtocol &&) noexcept = delete;
    SupProtocol &operator=(SupProtocol const &) = delete;

    SupProtocol(libp2p::Host &host,
                std::shared_ptr<blockchain::BlockTree> block_tree,
                std::shared_ptr<blockchain::BlockStorage> storage);

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
    outcome::result<Status> createStatus() const;

    void readStatus(std::shared_ptr<Stream> stream);

    void writeStatus(
        std::shared_ptr<Stream> stream,
        const Status &status,
        std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb);

    libp2p::Host &host_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<blockchain::BlockStorage> storage_;
    std::shared_ptr<StreamEngine> stream_engine_;
    const libp2p::peer::Protocol protocol_;
    log::Logger log_ = log::createLogger("SupProtocol");
  };

}  // namespace kagome::network

OUTCOME_HPP_DECLARE_ERROR(kagome::network, SupProtocol::Error);

#endif  // KAGOME_NETWORK_SUPPROTOCOL
