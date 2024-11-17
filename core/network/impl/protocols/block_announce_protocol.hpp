/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "injector/lazy.hpp"
#include "network/notifications/protocol.hpp"
#include "network/types/block_announce.hpp"
#include "network/types/block_announce_handshake.hpp"
#include "telemetry/peer_count.hpp"
#include "utils/lru.hpp"
#include "utils/non_copyable.hpp"

namespace kagome::application {
  class AppConfiguration;
  class ChainSpec;
}  // namespace kagome::application

namespace kagome::blockchain {
  class GenesisBlockHash;
  class BlockTree;
}  // namespace kagome::blockchain

namespace kagome::crypto {
  class Hasher;
}  // namespace kagome::crypto

namespace kagome::network {
  struct BlockAnnounceObserver;
  class GrandpaProtocol;
  class PeerManager;
  class PropagateTransactionsProtocol;
}  // namespace kagome::network

namespace kagome::network {
  using common::MainThreadPool;
  using libp2p::PeerId;

  class BlockAnnounceProtocol final
      : public std::enable_shared_from_this<BlockAnnounceProtocol>,
        public notifications::Controller,
        NonCopyable,
        NonMovable {
   public:
    BlockAnnounceProtocol(
        MainThreadPool &main_thread_pool,
        const application::AppConfiguration &app_config,
        const notifications::Factory &notifications_factory,
        Roles roles,
        const application::ChainSpec &chain_spec,
        const blockchain::GenesisBlockHash &genesis_hash,
        std::shared_ptr<blockchain::BlockTree> block_tree,
        std::shared_ptr<BlockAnnounceObserver> observer,
        LazySPtr<GrandpaProtocol> grandpa_protocol,
        LazySPtr<PropagateTransactionsProtocol> transaction_protocol,
        std::shared_ptr<crypto::Hasher> hasher,
        std::shared_ptr<telemetry::PeerCount> telemetry_peer_count,
        std::shared_ptr<PeerManager> peer_manager);

    // Controller
    Buffer handshake() override;
    bool onHandshake(const PeerId &peer_id,
                     size_t,
                     bool,
                     Buffer &&handshake) override;
    bool onMessage(const PeerId &peer_id, size_t, Buffer &&message) override;
    void onClose(const PeerId &peer_id) override;

    void start();
    void blockAnnounce(BlockAnnounce &&announce);

   private:
    std::shared_ptr<PoolHandler> main_pool_handler_;
    std::shared_ptr<notifications::Protocol> notifications_;
    BlockAnnounceHandshake handshake_;
    std::shared_ptr<blockchain::BlockTree> block_tree_;
    std::shared_ptr<BlockAnnounceObserver> observer_;
    LazySPtr<GrandpaProtocol> grandpa_protocol_;
    LazySPtr<PropagateTransactionsProtocol> transaction_protocol_;
    std::shared_ptr<crypto::Hasher> hasher_;
    std::shared_ptr<telemetry::PeerCount> telemetry_peer_count_;
    std::shared_ptr<PeerManager> peer_manager_;
    MapLruSet<PeerId, Hash256> seen_;
  };

}  // namespace kagome::network
