/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_NETWORK_BLOCKANNOUNCEPROTOCOL
#define KAGOME_NETWORK_BLOCKANNOUNCEPROTOCOL

#include "network/protocol_base.hpp"

#include <memory>

#include <libp2p/connection/stream.hpp>
#include <libp2p/host/host.hpp>

#include "application/app_configuration.hpp"
#include "application/chain_spec.hpp"
#include "blockchain/block_storage.hpp"
#include "blockchain/block_tree.hpp"
#include "containers/objects_cache.hpp"
#include "crypto/hasher.hpp"
#include "log/logger.hpp"
#include "network/block_announce_observer.hpp"
#include "network/impl/stream_engine.hpp"
#include "network/peer_manager.hpp"
#include "network/types/block_announce.hpp"
#include "network/types/status.hpp"

namespace kagome::network {

  KAGOME_DECLARE_CACHE(BlockAnnounceProtocol, KAGOME_CACHE_UNIT(BlockAnnounce));

  using Stream = libp2p::connection::Stream;
  using Protocol = libp2p::peer::Protocol;
  using PeerId = libp2p::peer::PeerId;
  using PeerInfo = libp2p::peer::PeerInfo;

  class BlockAnnounceProtocol final
      : public ProtocolBase,
        public std::enable_shared_from_this<BlockAnnounceProtocol> {
   public:
    BlockAnnounceProtocol() = delete;
    BlockAnnounceProtocol(BlockAnnounceProtocol &&) noexcept = delete;
    BlockAnnounceProtocol(const BlockAnnounceProtocol &) = delete;
    ~BlockAnnounceProtocol() override = default;
    BlockAnnounceProtocol &operator=(BlockAnnounceProtocol &&) noexcept =
        delete;
    BlockAnnounceProtocol &operator=(BlockAnnounceProtocol const &) = delete;

    BlockAnnounceProtocol(libp2p::Host &host,
                          const application::AppConfiguration &app_config,
                          const application::ChainSpec &chain_spec,
                          std::shared_ptr<StreamEngine> stream_engine,
                          std::shared_ptr<blockchain::BlockTree> block_tree,
                          std::shared_ptr<BlockAnnounceObserver> observer,
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

    void blockAnnounce(BlockAnnounce &&announce);

   private:
    outcome::result<Status> createStatus() const;

    enum class Direction { INCOMING, OUTGOING };
    void readStatus(std::shared_ptr<Stream> stream,
                    Direction direction,
                    std::function<void(outcome::result<void>)> &&cb);

    void writeStatus(std::shared_ptr<Stream> stream,
                     Direction direction,
                     std::function<void(outcome::result<void>)> &&cb);

    void readAnnounce(std::shared_ptr<Stream> stream);

    libp2p::Host &host_;
    const application::AppConfiguration &app_config_;
    std::shared_ptr<StreamEngine> stream_engine_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<BlockAnnounceObserver> observer_;
    std::shared_ptr<PeerManager> peer_manager_;
    const libp2p::peer::Protocol protocol_;
    log::Logger log_ =
        log::createLogger("BlockAnnounceProtocol", "kagome_protocols");
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_BLOCKANNOUNCEPROTOCOL
