/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/synchronizer_impl.hpp"

#include <random>

#include <libp2p/common/final_action.hpp>
#include <libp2p/common/shared_fn.hpp>
#include <libp2p/peer/peer_id.hpp>

#include "application/app_configuration.hpp"
#include "blockchain/block_tree.hpp"
#include "blockchain/block_tree_error.hpp"
#include "common/main_thread_pool.hpp"
#include "consensus/babe/impl/babe_digests_util.hpp"
#include "consensus/beefy/beefy.hpp"
#include "consensus/grandpa/environment.hpp"
#include "consensus/grandpa/has_authority_set_change.hpp"
#include "consensus/timeline/slots_util.hpp"
#include "consensus/timeline/timeline.hpp"
#include "network/peer_manager.hpp"
#include "network/protocols/state_protocol.hpp"
#include "network/protocols/sync_protocol.hpp"
#include "network/types/block_attributes.hpp"
#include "network/warp/protocol.hpp"
#include "primitives/common.hpp"
#include "primitives/extrinsic_root.hpp"
#include "storage/predefined_keys.hpp"
#include "storage/trie/serialization/trie_serializer.hpp"
#include "storage/trie/trie_batches.hpp"
#include "storage/trie/trie_storage.hpp"
#include "storage/trie_pruner/trie_pruner.hpp"
#include "utils/map_entry.hpp"
#include "utils/pool_handler_ready_make.hpp"
#include "utils/retain_if.hpp"
#include "utils/sptr.hpp"
#include "utils/weak_macro.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::network, SynchronizerImpl::Error, e) {
  using E = kagome::network::SynchronizerImpl::Error;
  switch (e) {
    case E::SHUTTING_DOWN:
      return "Node is shutting down";
    case E::EMPTY_RESPONSE:
      return "Response is empty";
    case E::RESPONSE_WITHOUT_BLOCK_HEADER:
      return "Response does not contain header of some block";
    case E::RESPONSE_WITHOUT_BLOCK_BODY:
      return "Response does not contain body of some block";
    case E::DISCARDED_BLOCK:
      return "Block is discarded";
    case E::WRONG_ORDER:
      return "Wrong order of blocks/headers in response";
    case E::INVALID_HASH:
      return "Hash does not match";
    case E::ALREADY_IN_QUEUE:
      return "Block is already enqueued";
    case E::PEER_BUSY:
      return "Peer is busy";
    case E::ARRIVED_TOO_EARLY:
      return "Block is arrived too early. Try to process it late";
    case E::DUPLICATE_REQUEST:
      return "Duplicate of recent request has been detected";
  }
  return "unknown error";
}

namespace {
  constexpr const char *kImportQueueLength =
      "kagome_import_queue_blocks_submitted";

  constexpr auto kRandomWarpInterval = std::chrono::minutes{1};

  /// restart synchronizer if no blocks are applied within specified time
  constexpr auto kHangTimer = std::chrono::minutes{5};

  constexpr kagome::consensus::SlotNumber kAllowedFutureBlockSlots = 2;
}  // namespace

namespace kagome::network {
  enum class VisitAncestryResult : uint8_t {
    CONTINUE,
    STOP,
    IGNORE_SUBTREE,
  };

  SynchronizerImpl::SynchronizerImpl(
      const application::AppConfiguration &app_config,
      application::AppStateManager &app_state_manager,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<clock::SystemClock> clock,
      LazySPtr<consensus::SlotsUtil> slots_util,
      std::shared_ptr<consensus::BlockHeaderAppender> block_appender,
      std::shared_ptr<consensus::BlockExecutor> block_executor,
      std::shared_ptr<storage::trie::TrieStorageBackend> trie_node_db,
      std::shared_ptr<storage::trie::TrieStorage> storage,
      std::shared_ptr<storage::trie_pruner::TriePruner> trie_pruner,
      std::shared_ptr<network::Router> router,
      std::shared_ptr<PeerManager> peer_manager,
      std::shared_ptr<libp2p::basic::Scheduler> scheduler,
      std::shared_ptr<crypto::Hasher> hasher,
      primitives::events::ChainSubscriptionEnginePtr chain_sub_engine,
      LazySPtr<consensus::Timeline> timeline,
      std::shared_ptr<Beefy> beefy,
      std::shared_ptr<consensus::grandpa::Environment> grandpa_environment,
      common::MainThreadPool &main_thread_pool,
      std::shared_ptr<blockchain::BlockStorage> block_storage)
      : log_(log::createLogger("Synchronizer", "synchronizer")),
        block_tree_(std::move(block_tree)),
        clock_{std::move(clock)},
        slots_util_{std::move(slots_util)},
        block_appender_(std::move(block_appender)),
        block_executor_(std::move(block_executor)),
        trie_node_db_(std::move(trie_node_db)),
        storage_(std::move(storage)),
        trie_pruner_(std::move(trie_pruner)),
        router_(std::move(router)),
        peer_manager_(std::move(peer_manager)),
        scheduler_(std::move(scheduler)),
        hasher_(std::move(hasher)),
        timeline_{timeline},
        beefy_{std::move(beefy)},
        grandpa_environment_{std::move(grandpa_environment)},
        chain_sub_engine_(std::move(chain_sub_engine)),
        main_pool_handler_{
            poolHandlerReadyMake(app_state_manager, main_thread_pool)},
        block_storage_{std::move(block_storage)},
        max_parallel_downloads_{app_config.maxParallelDownloads()},
        random_gen_{std::random_device{}()} {
    BOOST_ASSERT(block_tree_);
    BOOST_ASSERT(block_executor_);
    BOOST_ASSERT(trie_node_db_);
    BOOST_ASSERT(storage_);
    BOOST_ASSERT(trie_pruner_);
    BOOST_ASSERT(router_);
    BOOST_ASSERT(scheduler_);
    BOOST_ASSERT(hasher_);
    BOOST_ASSERT(grandpa_environment_);
    BOOST_ASSERT(chain_sub_engine_);
    BOOST_ASSERT(main_pool_handler_);
    BOOST_ASSERT(block_storage_);

    sync_method_ = app_config.syncMethod();

    // Register metrics
    metrics_registry_->registerGaugeFamily(
        kImportQueueLength, "Number of blocks submitted to the import queue");
    metric_import_queue_length_ =
        metrics_registry_->registerGaugeMetric(kImportQueueLength);
    metric_import_queue_length_->set(0);

    app_state_manager.takeControl(*this);
  }

  /** @see AppStateManager::takeControl */
  bool SynchronizerImpl::start() {
    randomWarp();
    setHangTimer();
    return true;
  }
  void SynchronizerImpl::stop() {
    node_is_shutting_down_ = true;
  }

  void SynchronizerImpl::addPeerKnownBlockInfo(const BlockInfo &block_info,
                                               const PeerId &peer_id) {
    if (state_sync_.has_value()) {
      return;
    }
    // If provided block is already enqueued, just remember peer
    if (auto known = entry(known_blocks_, block_info.hash)) {
      known->peers.emplace(peer_id);
    } else if (block_tree_->has(block_info.hash)) {
      return;
    }
    fetchTasks();
  }

  void SynchronizerImpl::onBlockAnnounce(const BlockHeader &header,
                                         const PeerId &peer_id) {
    if (state_sync_.has_value()) {
      return;
    }
    addHeader(peer_id, {.header = header});
    fetchTasks();
  }

  void SynchronizerImpl::loadBlocks(const PeerId &peer_id,
                                    BlocksRequest request,
                                    size_t &fetching,
                                    const char *reason) {
    ++fetching;
    busy_peers_.emplace(peer_id);
    auto busy = toSptr(
        libp2p::common::MovableFinalAction{[WEAK_SELF, peer_id, &fetching] {
          WEAK_LOCK(self);
          --fetching;
          self->busy_peers_.erase(peer_id);
        }});
    auto cb = [WEAK_SELF, peer_id, request, reason, busy{std::move(busy)}](
                  outcome::result<BlocksResponse> response_res) mutable {
      WEAK_LOCK(self);
      busy.reset();
      libp2p::common::FinalAction fetch_more{[&] { self->fetchTasks(); }};

      // Any error interrupts loading of blocks
      if (response_res.has_error()) {
        SL_VERBOSE(self->log_,
                   "Can't load blocks from {} beginning block {}: {}",
                   peer_id,
                   request.from,
                   response_res.error());
        return;
      }
      auto &response = response_res.value();
      if (not self->validate(request, response)) {
        SL_VERBOSE(self->log_,
                   "received invalid blocks for {} from {}",
                   reason,
                   peer_id);
        return;
      }
      SL_VERBOSE(self->log_,
                 "received {} blocks for {} from {}",
                 response.blocks.size(),
                 reason,
                 peer_id);
      for (auto &block : response.blocks) {
        if (block.header.has_value()) {
          if (not self->addHeader(peer_id, std::move(block))) {
            continue;
          }
        }
        if (block.body.has_value()) {
          if (auto known = entry(self->known_blocks_, block.hash)) {
            known->data.body = std::move(*block.body);
          }
        }
      }
      SL_TRACE(self->log_, "Block loading is finished");
    };
    fetch(peer_id, std::move(request), reason, std::move(cb));
  }

  void SynchronizerImpl::syncState(const BlockInfo &block,
                                   SyncStateCb handler) {
    std::unique_lock lock{state_sync_mutex_};
    if (state_sync_) {
      return;
    }
    auto peer = peer_manager_->peerFinalized(block.number, nullptr);
    if (not peer) {
      return;
    }
    auto _header = block_tree_->getBlockHeader(block.hash);
    if (not _header) {
      lock.unlock();
      return;
    }
    auto &header = _header.value();
    if (storage_->getEphemeralBatchAt(header.state_root)) {
      afterStateSync();
      lock.unlock();
      handler();
      return;
    }
    if (not state_sync_flow_ or state_sync_flow_->blockInfo() != block) {
      state_sync_flow_.emplace(trie_node_db_, block, header);
    }
    state_sync_.emplace(StateSync{
        .peer = *peer,
        .cb = std::move(handler),
    });
    SL_INFO(log_, "Sync of state for block {} has started", block);
    syncState();
  }

  void SynchronizerImpl::syncState() {
    SL_TRACE(log_,
             "State sync request has sent to {} for block {}",
             state_sync_->peer,
             state_sync_flow_->blockInfo());

    auto request = state_sync_flow_->nextRequest();

    auto protocol = router_->getStateProtocol();
    BOOST_ASSERT_MSG(protocol, "Router did not provide state protocol");

    auto response_handler = [wp{weak_from_this()}](auto &&_res) mutable {
      auto self = wp.lock();
      if (not self) {
        return;
      }
      std::unique_lock lock{self->state_sync_mutex_};
      auto block = self->state_sync_flow_->blockInfo();
      auto ok = self->syncState(lock, std::move(_res));
      if (not ok) {
        auto cb = std::move(self->state_sync_->cb);
        SL_WARN(self->log_, "State syncing failed with error: {}", ok.error());
        self->state_sync_.reset();
        lock.unlock();
        self->syncState(block, std::move(cb));
      }
    };

    protocol->request(
        state_sync_->peer, std::move(request), std::move(response_handler));
  }

  outcome::result<void> SynchronizerImpl::syncState(
      std::unique_lock<std::mutex> &lock,
      outcome::result<StateResponse> &&_res) {
    OUTCOME_TRY(res, std::move(_res));
    setHangTimer();
    OUTCOME_TRY(state_sync_flow_->onResponse(res));
    if (not state_sync_flow_->complete()) {
      syncState();
      return outcome::success();
    }
    OUTCOME_TRY(trie_pruner_->addNewState(state_sync_flow_->root(),
                                          storage::trie::StateVersion::V0));
    auto block = state_sync_flow_->blockInfo();
    state_sync_flow_.reset();
    SL_INFO(log_, "State syncing block {} has finished.", block);
    chain_sub_engine_->notify(primitives::events::ChainEventType::kNewRuntime,
                              block.hash);

    auto cb = std::move(state_sync_->cb);
    state_sync_.reset();

    // State syncing has completed; Switch to the full syncing
    afterStateSync();
    lock.unlock();
    cb();
    return outcome::success();
  }

  void SynchronizerImpl::applyNextBlock() {
    if (state_sync_.has_value()) {
      // Skip block application during state synchronization
      return;
    }
    for (auto &block_info : attached_roots_) {
      // Skip blocks that are already being processed
      if (executing_blocks_.contains(block_info)) {
        continue;
      }
      auto &known = known_blocks_.at(block_info.hash);

      // Create callback for handling block application result
      auto make_cb = [&] {
        // Mark block as being processed
        executing_blocks_.emplace(block_info);
        // Create destructor-like object to clean up executing_blocks_ when done
        auto dtor =
            toSptr(libp2p::common::MovableFinalAction{[WEAK_SELF, block_info] {
              WEAK_LOCK(self);
              self->executing_blocks_.erase(block_info);
            }});

        // Return callback that handles success/failure of block application
        return [WEAK_SELF, block_info, dtor](
                   outcome::result<void> result) mutable {
          WEAK_LOCK(self);
          // Schedule a task to process the result asynchronously
          self->scheduler_->schedule(
              [weak_self, block_info, dtor, result]() mutable {
                WEAK_LOCK(self);
                // Clean up executing_blocks_ entry
                dtor.reset();
                if (result.has_value()) {
                  // On success: notify telemetry about block import
                  self->telemetry_->notifyBlockImported(
                      block_info, telemetry::BlockOrigin::kNetworkInitialSync);
                } else {
                  // On failure: log error and remove the invalid block
                  SL_ERROR(self->log_,
                           "apply block {} error: {}",
                           block_info,
                           result.error());
                  if (self->attached_roots_.erase(block_info) != 0) {
                    // Remove the block and all its descendants from the queue
                    self->removeBlockRecursive(block_info);
                  }
                }
                // Update the roots after processing this block
                self->updateRoots();
              });
        };
      };

      auto &header = known.data.header.value();

      if (sync_method_ == application::SyncMethod::Full) {
        // For full synchronization, we need both header and body
        if (not known.data.body.has_value()) {
          // Skip blocks with missing bodies, they'll be fetched later
          continue;
        }
        // Apply full block (header + body + optional justification)
        block_executor_->applyBlock(
            {
                .header = header,
                .body = known.data.body.value(),
            },
            known.data.justification,
            make_cb());
      } else {
        // For header-only synchronization, just append the header
        block_appender_->appendHeader(
            BlockHeader{header}, known.data.justification, make_cb());
      }
    }
  }

  void SynchronizerImpl::updateRoots() {
    retain_if(attached_roots_, [&](const BlockInfo &root) {
      if (executing_blocks_.contains(root)) {
        return true;
      }
      auto &known = known_blocks_.at(root.hash);
      if (block_tree_->has(root.hash)) {
        if (known.data.beefy_justification) {
          beefy_->onJustification(root.hash,
                                  std::move(*known.data.beefy_justification));
        }
        for (auto [it, end] = ancestry_.equal_range(root); it != end; ++it) {
          attached_roots_.emplace(it->second);
        }
        removeBlock(root);
      } else {
        auto &header = known.data.header.value();
        if (block_tree_->has(header.parent_hash) and isBlockAllowed(header)) {
          return true;
        }
        removeBlockRecursive(root);
      }
      return false;
    });
    retain_if(detached_roots_, [&](const BlockInfo &root) {
      auto &known = known_blocks_.at(root.hash);
      if (isBlockAllowed(known.data.header.value())) {
        return true;
      }
      removeBlockRecursive(root);
      return false;
    });
    metric_import_queue_length_->set(known_blocks_.size());
    fetchTasks();
  }

  void SynchronizerImpl::scheduleRecentRequestRemoval(
      const libp2p::peer::PeerId &peer_id,
      const BlocksRequest::Fingerprint &fingerprint) {
    scheduler_->schedule(
        [wp{weak_from_this()}, peer_id, fingerprint] {
          if (auto self = wp.lock()) {
            self->recent_requests_.erase(std::tuple(peer_id, fingerprint));
          }
        },
        kRecentnessDuration);
  }

  void SynchronizerImpl::fetch(
      const libp2p::peer::PeerId &peer,
      BlocksRequest request,
      const char *reason,
      std::function<void(outcome::result<BlocksResponse>)> &&cb) {
    if (node_is_shutting_down_) {
      cb(Error::SHUTTING_DOWN);
      return;
    }
    auto fingerprint = request.fingerprint();
    if (not recent_requests_.emplace(std::tuple{peer, fingerprint}, reason)
                .second) {
      cb(Error::DUPLICATE_REQUEST);
      return;
    }
    scheduleRecentRequestRemoval(peer, fingerprint);
    router_->getSyncProtocol()->request(
        peer, std::move(request), std::move(cb));
  }

  std::optional<libp2p::peer::PeerId> SynchronizerImpl::chooseJustificationPeer(
      primitives::BlockNumber block, BlocksRequest::Fingerprint fingerprint) {
    return peer_manager_->peerFinalized(block, [&](const PeerId &peer) {
      if (busy_peers_.contains(peer)) {
        return false;
      }
      if (recent_requests_.contains({peer, fingerprint})) {
        return false;
      }
      return true;
    });
  }

  bool SynchronizerImpl::fetchJustification(const primitives::BlockInfo &block,
                                            CbResultVoid cb) {
    BlocksRequest request{
        .fields = BlockAttribute::JUSTIFICATION,
        .from = block.hash,
        .direction = Direction::DESCENDING,
        .max = 1,
        .multiple_justifications = false,
    };
    auto chosen = chooseJustificationPeer(block.number, request.fingerprint());
    if (not chosen) {
      return false;
    }
    busy_peers_.emplace(*chosen);
    auto cb2 = [weak{weak_from_this()},
                block,
                cb{std::move(cb)},
                peer{*chosen}](outcome::result<BlocksResponse> res) mutable {
      auto self = weak.lock();
      if (not self) {
        return;
      }
      self->busy_peers_.erase(peer);
      if (not res) {
        return cb(res.error());
      }
      auto &blocks = res.value().blocks;
      if (blocks.size() != 1) {
        return cb(Error::EMPTY_RESPONSE);
      }
      auto &justification = blocks[0].justification;
      if (not justification) {
        return cb(Error::EMPTY_RESPONSE);
      }
      self->grandpa_environment_->applyJustification(
          block, *justification, std::move(cb));
    };
    fetch(*chosen, std::move(request), "justification", std::move(cb2));
    return true;
  }

  bool SynchronizerImpl::fetchJustificationRange(primitives::BlockNumber min,
                                                 FetchJustificationRangeCb cb) {
    auto hash_res = block_tree_->getHashByNumber(min);
    if (not hash_res) {
      return false;
    }
    auto &hash = hash_res.value();
    auto chosen = chooseJustificationPeer(min, min);
    if (not chosen) {
      return false;
    }
    busy_peers_.emplace(*chosen);
    auto cb2 = [weak{weak_from_this()}, min, cb{std::move(cb)}, peer{*chosen}](
                   outcome::result<WarpResponse> res) mutable {
      auto self = weak.lock();
      if (not self) {
        return;
      }
      self->busy_peers_.erase(peer);
      if (not res) {
        return cb(res.error());
      }
      auto &blocks = res.value().proofs;
      for (const auto &block : blocks) {
        self->grandpa_environment_->applyJustification(
            block.justification.block_info,
            {scale::encode(block.justification).value()},
            [cb{std::move(cb)}](outcome::result<void> res) {
              if (not res) {
                cb(res.error());
              } else {
                cb(std::nullopt);
              }
            });
        return;
      }
      cb(min);
    };
    router_->getWarpProtocol()->doRequest(*chosen, hash, std::move(cb2));
    return true;
  }

  void SynchronizerImpl::afterStateSync() {
    sync_method_ = application::SyncMethod::Full;
    known_blocks_.clear();
    attached_roots_.clear();
    detached_roots_.clear();
    ancestry_.clear();
    recent_requests_.clear();
    metric_import_queue_length_->set(known_blocks_.size());
  }

  bool SynchronizerImpl::fetchHeadersBack(const primitives::BlockInfo &max,
                                          primitives::BlockNumber min,
                                          bool isFinalized,
                                          CbResultVoid cb) {
    auto initialBlockNumber = max.number;
    if (initialBlockNumber < min) {
      return false;
    }

    BlocksRequest request{
        .fields = BlockAttribute::HEADER,
        .from = initialBlockNumber,
        .direction = Direction::DESCENDING,
        .max = initialBlockNumber - min + 1,
        .multiple_justifications = false,
    };
    auto chosen =
        chooseJustificationPeer(initialBlockNumber, request.fingerprint());
    if (not chosen) {
      return false;
    }
    auto expected = max;

    busy_peers_.emplace(*chosen);
    auto cb2 = [weak{weak_from_this()},
                expected,
                isFinalized,
                cb{std::move(cb)},
                peer{*chosen}](outcome::result<BlocksResponse> res) mutable {
      auto self = weak.lock();
      if (not self) {
        return cb(Error::SHUTTING_DOWN);
      }

      self->busy_peers_.erase(peer);
      if (not res) {
        return cb(res.error());
      }

      auto &blocks = res.value().blocks;
      if (blocks.empty()) {
        return cb(Error::EMPTY_RESPONSE);
      }
      for (auto &block : blocks) {
        auto &header = block.header;

        if (not header) {
          return cb(Error::EMPTY_RESPONSE);
        }

        auto &headerValue = header.value();
        primitives::calculateBlockHash(headerValue, *self->hasher_);
        const auto &headerInfo = headerValue.blockInfo();

        if (headerInfo != expected) {
          SL_ERROR(self->log_,
                   "Header info is different from expected, block #{}",
                   expected.number);
          return cb(Error::INVALID_HASH);
        }

        if (auto res = self->block_storage_->putBlockHeader(headerValue);
            res.has_error()) {
          SL_ERROR(self->log_, "Failed to put block header: {}", res.error());
          return cb(res.error());
        }

        if (isFinalized) {
          if (auto res = self->block_storage_->assignNumberToHash(headerInfo);
              res.has_error()) {
            SL_ERROR(
                self->log_, "Failed to assign number to hash: {}", res.error());
            return cb(res.error());
          }
        }
        const auto headerNumber = headerInfo.number;
        SL_TRACE(self->log_, "Block #{} is successfully stored", headerNumber);
        if (const auto parentInfo = headerValue.parentInfo(); parentInfo) {
          expected = *parentInfo;
        } else if (headerNumber == 0) {
          break;
        } else {
          SL_ERROR(self->log_,
                   "Parent info is not provided for block #{}",
                   headerNumber);
          return cb(Error::EMPTY_RESPONSE);
        }
      }
      return cb(outcome::success());
    };

    fetch(*chosen, std::move(request), "header", std::move(cb2));
    return true;
  }

  void SynchronizerImpl::trySyncShortFork(const PeerId &peer_id,
                                          const primitives::BlockInfo &block) {
    BlocksRequest request{
        .fields = BlockAttribute::HEADER,
        .from = block.hash,
        .direction = Direction::DESCENDING,
    };
    static size_t fetching = 0;
    loadBlocks(peer_id, std::move(request), fetching, "grandpa-fork");
  }

  void SynchronizerImpl::randomWarp() {
    scheduler_->schedule(
        [WEAK_SELF] {
          WEAK_LOCK(self);
          self->randomWarp();
        },
        kRandomWarpInterval);
    if (not timeline_.get()->wasSynchronized()) {
      return;
    }
    auto finalized = block_tree_->getLastFinalized();
    auto cb = [WEAK_SELF, finalized](outcome::result<WarpResponse> r) mutable {
      WEAK_LOCK(self);
      if (not r) {
        return;
      }
      auto &blocks = r.value().proofs;
      for (const auto &block : blocks) {
        if (block.header.number == finalized.number) {
          continue;
        }
        SL_INFO(self->log_, "randomWarp justification {}", block.header.number);
        self->grandpa_environment_->applyJustification(
            block.justification.block_info,
            {scale::encode(block.justification).value()},
            [](outcome::result<void> r) {});
      }
    };
    router_->getWarpProtocol()->random(finalized.hash, cb);
  }

  void SynchronizerImpl::unsafe(PeerId peer_id, BlockNumber max, UnsafeCb cb) {
    BlocksRequest request{
        .fields = BlockAttribute::HEADER | BlockAttribute::JUSTIFICATION,
        .from = max,
        .direction = Direction::DESCENDING,
        .max = std::nullopt,
        .multiple_justifications = false,
    };
    auto cb2 = [WEAK_SELF, max, cb{std::move(cb)}](
                   outcome::result<BlocksResponse> res) mutable {
      WEAK_LOCK(self);
      if (not res) {
        return;
      }
      auto &blocks = res.value().blocks;
      if (blocks.empty()) {
        return;
      }
      auto next = max;
      for (auto &block : blocks) {
        if (not block.header) {
          break;
        }
        if (block.header->number != next) {
          break;
        }
        if (block.header->number == 0) {
          break;
        }
        --next;
        consensus::grandpa::HasAuthoritySetChange digest{*block.header};
        if (digest.scheduled && block.justification) {
          primitives::calculateBlockHash(*block.header, *self->hasher_);
          auto justification_res =
              scale::decode<GrandpaJustification>(block.justification->data);
          if (not justification_res) {
            break;
          }
          auto &justification = justification_res.value();
          if (justification.block_info != block.header->blockInfo()) {
            break;
          }
          cb(std::make_pair(*block.header, justification));
          return;
        }
      }
      if (next == max) {
        return;
      }
      cb(next);
    };
    fetch(peer_id, std::move(request), "unsafe", std::move(cb2));
  }

  void SynchronizerImpl::setHangTimer() {
    hang_timer_ = scheduler_->scheduleWithHandle(
        [WEAK_SELF] {
          WEAK_LOCK(self);
          self->onHangTimer();
        },
        kHangTimer);
  }

  void SynchronizerImpl::onHangTimer() {
    SL_WARN(log_, "sync hung up, restarting sync");
    known_blocks_.clear();
    attached_roots_.clear();
    detached_roots_.clear();
    ancestry_.clear();
  }

  bool SynchronizerImpl::addHeader(const PeerId &peer_id,
                                   primitives::BlockData &&data) {
    auto &header = data.header.value();
    if (header.number > highestAllowedBlock()) {
      return false;
    }
    // save info before moving `data`
    auto block_info = header.blockInfo();
    auto parent_info = header.parentInfo();
    if (not parent_info.has_value()) {
      return false;
    }
    auto known = entry(known_blocks_, block_info.hash);
    if (not known) {
      if (isFutureBlock(data.header.value())) {
        return false;
      }
      if (block_tree_->has(block_info.hash)) {
        return false;
      }
      auto allow_block = isBlockAllowed(header);
      if (allow_block) {
        if (auto parent_res = block_tree_->getBlockHeader(parent_info->hash)) {
          auto &parent = parent_res.value();
          if (not isSlotIncreasing(parent, data.header.value())) {
            allow_block = false;
          } else {
            attached_roots_.emplace(block_info);
          }
        } else if (auto parent = entry(known_blocks_, parent_info->hash)) {
          if (not isSlotIncreasing(parent->data.header.value(),
                                   data.header.value())) {
            allow_block = false;
          }
        } else {
          detached_roots_.emplace(block_info);
        }
      }
      // Skip headers of blocks that don't meet the validation criteria
      // and clean up any child blocks that might already be in our tree
      if (not allow_block) {
        // Find all child blocks of this disallowed block in the ancestry map
        for (auto [it, end] = ancestry_.equal_range(block_info); it != end;) {
          auto child = it->second;
          ++it;
          // Remove the child from detached roots since its parent is invalid
          detached_roots_.erase(child);
          // Recursively remove the child and its descendants from the known
          // blocks
          removeBlockRecursive(child);
        }
        return false;  // Indicate that the header was not added
      }
      known.insert({
          .data = std::move(data),
          .ancestry_it = ancestry_.emplace(*parent_info, block_info),
      });
      metric_import_queue_length_->set(known_blocks_.size());
      for (auto [it, end] = ancestry_.equal_range(block_info); it != end;) {
        auto it2 = std::next(it);
        detached_roots_.erase(it->second);
        if (not isSlotIncreasing(
                known->data.header.value(),
                known_blocks_.at(it->second.hash).data.header.value())) {
          removeBlockRecursive(it->second);
        }
        it = it2;
      }
    }
    known->peers.emplace(peer_id);
    setHangTimer();
    return true;
  }

  void SynchronizerImpl::fetchTasks() {
    // Lambda to select a peer from which to request blocks
    auto choose_peer = [&](const BlockInfo &block, size_t fingerprint) {
      std::optional<PeerId> chosen_peer;
      visitAncestry(block, [&](const KnownBlock &block) {
        for (auto &peer_id : block.peers) {
          if (not busy_peers_.contains(peer_id)
              and not recent_requests_.contains({peer_id, fingerprint})) {
            chosen_peer = peer_id;
            return VisitAncestryResult::STOP;
          }
        }
        return VisitAncestryResult::CONTINUE;
      });
      return chosen_peer;
    };
    if (sync_method_ == application::SyncMethod::Full) {
      // Fetches block bodies for blocks in attached_roots_ that have headers
      // but no bodies This loop processes each attached root and traverses its
      // ancestry tree to find blocks that need their bodies fetched from peers
      for (auto &root : attached_roots_) {
        // Check if we've reached the parallel download limit before continuing
        if (not canFetchMore(fetching_body_count_)) {
          break;
        }

        // For each root, traverse the ancestry tree to look for blocks that
        // need bodies
        visitAncestry(root, [&](const KnownBlock &block) {
          // Stop traversal if we've hit the download limit during traversal
          if (not canFetchMore(fetching_body_count_)) {
            return VisitAncestryResult::STOP;
          }

          // If this block has a header but no body, try to fetch its body
          if (not block.data.body.has_value()) {
            auto block_info = block.data.header.value().blockInfo();

            // Create a request for the block body
            BlocksRequest request{
                .fields = BlockAttribute::HEADER | BlockAttribute::BODY,
                .from = block_info.hash,
                .direction = Direction::ASCENDING,
            };

            // Find a suitable peer that knows about this block and isn't busy
            if (auto chosen_peer =
                    choose_peer(block_info, request.fingerprint())) {
              SL_VERBOSE(
                  log_, "fetch body {} from {}", block_info, *chosen_peer);

              // Request the block body from the chosen peer
              loadBlocks(*chosen_peer,
                         std::move(request),
                         fetching_body_count_,
                         "body");
            }
            // Skip the subtree after requesting a body - we want to process
            // this block and its ancestors fully before moving on to its
            // descendants
            return VisitAncestryResult::IGNORE_SUBTREE;
          }

          // Continue traversal for blocks that already have bodies
          return VisitAncestryResult::CONTINUE;
        });
      }
    }

    // Fetches headers and justifications for detached blocks in the blockchain
    // This helps "fill the gap" between detached blocks and the main chain
    for (auto &root : detached_roots_) {
      if (not canFetchMore(fetching_gap_count_)) {
        break;
      }
      BlocksRequest request{
          .fields = BlockAttribute::HEADER | BlockAttribute::JUSTIFICATION,
          .from = known_blocks_.at(root.hash).data.header.value().parent_hash,
          .direction = Direction::DESCENDING,
      };
      if (auto chosen_peer = choose_peer(root, request.fingerprint())) {
        SL_VERBOSE(log_, "fetch gap {} from {}", root, *chosen_peer);
        loadBlocks(
            *chosen_peer, std::move(request), fetching_gap_count_, "gap");
      }
    }

    // Fetch block ranges from peers when we have capacity
    if (canFetchMore(fetching_range_count_)) {
      // Start with the best block number we know from the main chain
      BlockNumber max_attached = block_tree_->bestBlock().number;

      // Find the highest block number among all attached blocks and their
      // descendants
      for (auto &root : attached_roots_) {
        visitAncestry(root, [&](const KnownBlock &block) {
          max_attached =
              std::max(max_attached, block.data.header.value().number);
          return VisitAncestryResult::CONTINUE;
        });
      }

      // Only fetch if we haven't reached our maximum allowed block height
      if (max_attached < highestAllowedBlock()) {
        // Prepare a request to get blocks after our highest known block
        BlocksRequest request{
            .fields = BlockAttribute::HEADER | BlockAttribute::JUSTIFICATION,
            .from = max_attached,
            .direction =
                Direction::ASCENDING,  // Get blocks newer than max_attached
        };

        // Look through all peers to find candidates for fetching
        peer_manager_->enumeratePeerState([&](const PeerId &peer_id,
                                              PeerState &state) {
          // Stop if we've reached parallel download limit during enumeration
          if (not canFetchMore(fetching_range_count_)) {
            return false;
          }

          // Check if peer has newer blocks and isn't already busy with another
          // request Also avoid duplicate requests to the same peer
          if (state.best_block.number >= max_attached
              and not busy_peers_.contains(peer_id)
              and not recent_requests_.contains(
                  {peer_id, request.fingerprint()})) {
            SL_VERBOSE(log_, "fetch range {} from {}", max_attached, peer_id);
            loadBlocks(peer_id, request, fetching_range_count_, "range");
          }
          return true;  // Continue to check other peers
        });
      }
    }
    applyNextBlock();
  }

  void SynchronizerImpl::visitAncestry(const BlockInfo &root, const auto &f) {
    std::deque<BlockInfo> stack{root};
    while (not stack.empty()) {
      auto block = stack.back();
      stack.pop_back();
      switch (f(known_blocks_.at(block.hash))) {
        case VisitAncestryResult::CONTINUE: {
          for (auto [it, end] = ancestry_.equal_range(block); it != end; ++it) {
            stack.emplace_back(it->second);
          }
          break;
        }
        case VisitAncestryResult::STOP: {
          return;
        }
        case VisitAncestryResult::IGNORE_SUBTREE: {
          break;
        }
      }
    }
  }

  bool SynchronizerImpl::canFetchMore(size_t &fetching) const {
    return fetching < max_parallel_downloads_;
  }

  bool SynchronizerImpl::validate(const BlocksRequest &request,
                                  const BlocksResponse &response) const {
    auto &blocks = response.blocks;
    if (blocks.empty()) {
      return false;
    }
    auto need_header = has(request.fields, BlockAttribute::HEADER);
    auto need_body = has(request.fields, BlockAttribute::BODY);
    if (auto *hash = boost::get<BlockHash>(&request.from)) {
      if (blocks[0].hash != *hash) {
        return false;
      }
    } else {
      auto number = boost::get<BlockNumber>(request.from);
      if (blocks[0].header.has_value() and blocks[0].header->number != number) {
        return false;
      }
    }
    for (auto &block : blocks) {
      if (block.header.has_value() != need_header) {
        return false;
      }
      if (not need_body and block.body.has_value()) {
        return false;
      }
      if (need_header) {
        calculateBlockHash(block.header.value(), *hasher_);
        if (block.hash != block.header->hash()) {
          return false;
        }
      }
    }
    if (need_header) {
      auto validate_chain = [&](auto &&blocks) {
        std::optional<BlockInfo> expected;
        size_t count = 0;
        for (auto &block : blocks) {
          auto &header = block.header.value();
          ++count;
          if (expected.has_value() and header.blockInfo() != expected) {
            return false;
          }
          expected = header.parentInfo();
          if (not expected.has_value() and count != blocks.size()) {
            return false;
          }
        }
        return true;
      };
      if (request.direction == Direction::ASCENDING) {
        if (not validate_chain(blocks | std::views::reverse)) {
          return false;
        }
      } else {
        if (not validate_chain(blocks)) {
          return false;
        }
      }
    }
    if (need_body) {
      size_t count = 0;
      for (auto &block : blocks) {
        ++count;
        if (not block.body.has_value()) {
          if (count != blocks.size()) {
            return false;
          }
          continue;
        }
        if (auto known = entry(known_blocks_, block.hash)) {
          if (not checkExtrinsicRoot(known->data.header.value(), *block.body)) {
            return false;
          }
        }
      }
    }
    return true;
  }

  size_t SynchronizerImpl::highestAllowedBlock() const {
    return block_tree_->bestBlock().number
         + (sync_method_ == application::SyncMethod::Full
                ? kMinPreloadedBlockAmount
                : kMinPreloadedBlockAmountForFastSyncing);
  }

  bool SynchronizerImpl::isBlockAllowed(const BlockHeader &header) const {
    auto finalized = block_tree_->getLastFinalized();
    if (header.number <= finalized.number) {
      return false;
    }
    if (header.number == finalized.number + 1
        and header.parent_hash != finalized.hash) {
      return false;
    }
    return true;
  }

  void SynchronizerImpl::removeBlock(const BlockInfo &block) {
    auto known = entry(known_blocks_, block.hash).remove();
    ancestry_.erase(known.ancestry_it);
  }

  void SynchronizerImpl::removeBlockRecursive(const BlockInfo &block) {
    visitAncestry(block, [&](const KnownBlock &block) {
      removeBlock(block.data.header.value().blockInfo());
      return VisitAncestryResult::CONTINUE;
    });
  }

  bool SynchronizerImpl::isSlotIncreasing(const BlockHeader &parent,
                                          const BlockHeader &header) const {
    if (auto pre_result = consensus::babe::getBabeBlockHeader(header)) {
      auto &pre = pre_result.value();
      if (parent.number == 0) {
        return true;
      }
      if (auto parent_pre_result =
              consensus::babe::getBabeBlockHeader(parent)) {
        auto &parent_pre = parent_pre_result.value();
        return parent_pre.slot_number < pre.slot_number;
      }
    }
    return false;
  }

  bool SynchronizerImpl::isFutureBlock(const BlockHeader &header) const {
    if (auto pre_result = consensus::babe::getBabeBlockHeader(header)) {
      auto &pre = pre_result.value();
      auto allowed_slot = slots_util_.get()->timeToSlot(clock_->now())
                        + kAllowedFutureBlockSlots;
      return pre.slot_number > allowed_slot;
    }
    return true;
  }
}  // namespace kagome::network
