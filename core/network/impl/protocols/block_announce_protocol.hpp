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
#include "network/impl/protocols/protocol_base_impl.hpp"
#include "network/impl/stream_engine.hpp"
#include "network/peer_manager.hpp"
#include "network/types/block_announce.hpp"
#include "network/types/block_announce_handshake.hpp"
#include "utils/non_copyable.hpp"

namespace kagome::network {

  KAGOME_DECLARE_CACHE(BlockAnnounceProtocol, KAGOME_CACHE_UNIT(BlockAnnounce));

  class BlockAnnounceProtocol final
      : public ProtocolBase,
        public std::enable_shared_from_this<BlockAnnounceProtocol>,
        NonCopyable,
        NonMovable {
   public:
    BlockAnnounceProtocol() = delete;
    ~BlockAnnounceProtocol() override = default;

    BlockAnnounceProtocol(libp2p::Host &host,
                          const application::AppConfiguration &app_config,
                          const application::ChainSpec &chain_spec,
                          const primitives::BlockHash &genesis_hash,
                          std::shared_ptr<StreamEngine> stream_engine,
                          std::shared_ptr<blockchain::BlockTree> block_tree,
                          std::shared_ptr<BlockAnnounceObserver> observer,
                          std::shared_ptr<PeerManager> peer_manager);

    bool start() override;
    bool stop() override;

    const std::string &protocolName() const override;

    void onIncomingStream(std::shared_ptr<Stream> stream) override;
    void newOutgoingStream(
        const PeerInfo &peer_info,
        std::function<void(outcome::result<std::shared_ptr<Stream>>)> &&cb)
        override;

    void blockAnnounce(BlockAnnounce &&announce);

   private:
    outcome::result<BlockAnnounceHandshake> createHandshake() const;

    enum class Direction { INCOMING, OUTGOING };

    friend inline std::string_view to_string(Direction direction) {
      switch (direction) {
        case Direction::INCOMING:
          return "incoming";
        case Direction::OUTGOING:
          return "outgoing";
      }
    }

    void readHandshake(std::shared_ptr<Stream> handshake_res,
                       Direction direction,
                       std::function<void(outcome::result<void>)> &&cb);

    void writeHandshake(std::shared_ptr<Stream> stream,
                        Direction direction,
                        std::function<void(outcome::result<void>)> &&cb);

    void readAnnounce(std::shared_ptr<Stream> stream);

    const static inline auto kBlockAnnounceProtocolName =
        "BlockAnnounceProtocol"s;
    ProtocolBaseImpl base_;
    const application::AppConfiguration &app_config_;
    std::shared_ptr<StreamEngine> stream_engine_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<BlockAnnounceObserver> observer_;
    std::shared_ptr<PeerManager> peer_manager_;
  };

}  // namespace kagome::network

#endif  // KAGOME_NETWORK_BLOCKANNOUNCEPROTOCOL
