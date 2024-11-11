/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/protocols/block_announce_protocol.hpp"

#include "application/app_configuration.hpp"
#include "application/chain_spec.hpp"
#include "blockchain/block_tree.hpp"
#include "blockchain/genesis_block_hash.hpp"
#include "common/main_thread_pool.hpp"
#include "crypto/hasher.hpp"
#include "metrics/histogram_timer.hpp"
#include "network/block_announce_observer.hpp"
#include "network/common.hpp"
#include "network/impl/protocols/grandpa_protocol.hpp"
#include "network/impl/protocols/propagate_transactions_protocol.hpp"
#include "network/notifications/encode.hpp"
#include "network/peer_manager.hpp"
#include "utils/try.hpp"
#include "utils/weak_macro.hpp"

namespace kagome::network {
  // https://github.com/paritytech/polkadot-sdk/blob/edf79aa972bcf2e043e18065a9bb860ecdbd1a6e/substrate/client/network/sync/src/engine.rs#L86
  constexpr size_t kSeenCapacity = 1024;

  static const metrics::GaugeHelper metric_peers{
      "kagome_sync_peers",
      "Number of peers we sync with",
  };

  BlockAnnounceProtocol::BlockAnnounceProtocol(
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
      telemetry::PeerCount telemetry_peer_count,
      std::shared_ptr<PeerManager> peer_manager)
      : main_pool_handler_{main_thread_pool.handlerStarted()},
        notifications_{notifications_factory.make(
            {make_protocols(kBlockAnnouncesProtocol, genesis_hash, chain_spec)},
            app_config.inPeers(),
            app_config.outPeers())},
        handshake_{.roles = roles, .genesis_hash = genesis_hash},
        block_tree_(std::move(block_tree)),
        observer_(std::move(observer)),
        grandpa_protocol_(std::move(grandpa_protocol)),
        transaction_protocol_(std::move(transaction_protocol)),
        hasher_(std::move(hasher)),
        telemetry_peer_count_(std::move(telemetry_peer_count)),
        peer_manager_{std::move(peer_manager)},
        seen_{kSeenCapacity} {
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(observer_ != nullptr);
    BOOST_ASSERT(peer_manager_ != nullptr);
  }

  Buffer BlockAnnounceProtocol::handshake() {
    auto handshake = handshake_;
    handshake.best_block = block_tree_->bestBlock();
    return scale::encode(handshake).value();
  }

  bool BlockAnnounceProtocol::onHandshake(const PeerId &peer_id,
                                          size_t,
                                          bool,
                                          Buffer &&handshake_raw) {
    auto handshake =
        TRY_FALSE(scale::decode<BlockAnnounceHandshake>(handshake_raw));
    if (handshake.genesis_hash != block_tree_->getGenesisBlockHash()) {
      return false;
    }
    if (seen_.add(peer_id)) {
      metric_peers->inc(1);
      ++*telemetry_peer_count_.v;
    }
    grandpa_protocol_.get()->notifications_->reserve(peer_id, true);
    transaction_protocol_.get()->notifications_->reserve(peer_id, true);
    main_pool_handler_->execute(
        [WEAK_SELF, peer_id, handshake{std::move(handshake)}] {
          WEAK_LOCK(self);
          self->peer_manager_->updatePeerState(peer_id, handshake);
          self->observer_->onBlockAnnounceHandshake(peer_id, handshake);
          self->peer_manager_->startPingingPeer(peer_id);
        });
    return true;
  }

  bool BlockAnnounceProtocol::onMessage(const PeerId &peer_id,
                                        size_t,
                                        Buffer &&message_raw) {
    auto block_announce = TRY_FALSE(scale::decode<BlockAnnounce>(message_raw));
    primitives::calculateBlockHash(block_announce.header, *hasher_);
    if (not seen_.add(peer_id, block_announce.header.hash())) {
      return true;
    }
    main_pool_handler_->execute(
        [WEAK_SELF, peer_id, block_announce{std::move(block_announce)}] {
          WEAK_LOCK(self);
          self->peer_manager_->updatePeerState(peer_id, block_announce);
          self->observer_->onBlockAnnounce(peer_id, block_announce);
        });
    return true;
  }

  void BlockAnnounceProtocol::onClose(const PeerId &peer_id) {
    metric_peers->dec(1);
    --*telemetry_peer_count_.v;
    seen_.remove(peer_id);
    grandpa_protocol_.get()->notifications_->reserve(peer_id, false);
    transaction_protocol_.get()->notifications_->reserve(peer_id, false);
  }

  void BlockAnnounceProtocol::start() {
    notifications_->start(weak_from_this());
  }

  void BlockAnnounceProtocol::blockAnnounce(BlockAnnounce &&announce) {
    REINVOKE(*main_pool_handler_, blockAnnounce, std::move(announce));
    auto message_raw = notifications::encode(announce);
    notifications_->peersOut([&](const PeerId &peer_id, size_t) {
      if (not seen_.add(peer_id, announce.header.hash())) {
        return true;
      }
      notifications_->write(peer_id, message_raw);
      return true;
    });
  }
}  // namespace kagome::network
