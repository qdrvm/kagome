/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_SUPPROTOCOL
#define KAGOME_NETWORK_SUPPROTOCOL

#include "network/protocol_base.hpp"

#include <memory>

#include <libp2p/connection/stream.hpp>
#include <libp2p/host/host.hpp>

#include "application/app_configuration.hpp"
#include "blockchain/block_storage.hpp"
#include "blockchain/block_tree.hpp"
#include "log/logger.hpp"
#include "network/impl/stream_engine.hpp"
#include "network/peer_manager.hpp"
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
    SupProtocol() = delete;
    SupProtocol(SupProtocol &&) noexcept = delete;
    SupProtocol(const SupProtocol &) = delete;
    ~SupProtocol() override = default;
    SupProtocol &operator=(SupProtocol &&) noexcept = delete;
    SupProtocol &operator=(SupProtocol const &) = delete;

    SupProtocol(libp2p::Host &host,
                const application::AppConfiguration &app_config,
                std::shared_ptr<StreamEngine> stream_engine,
                std::shared_ptr<blockchain::BlockTree> block_tree,
                std::shared_ptr<blockchain::BlockStorage> storage,
                std::shared_ptr<PeerManager> peer_manager);

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
    const application::AppConfiguration &app_config_;
    std::shared_ptr<StreamEngine> stream_engine_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<blockchain::BlockStorage> storage_;
    std::shared_ptr<PeerManager> peer_manager_;
    const libp2p::peer::Protocol protocol_;
    log::Logger log_ = log::createLogger("SupProtocol", "protocols");
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_SUPPROTOCOL
