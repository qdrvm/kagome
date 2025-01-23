/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/synchronizer_impl.hpp"

#include <random>

#include <libp2p/common/final_action.hpp>
#include <libp2p/peer/peer_id.hpp>

#include "application/app_configuration.hpp"
#include "blockchain/block_tree.hpp"
#include "blockchain/block_tree_error.hpp"
#include "common/main_thread_pool.hpp"
#include "consensus/beefy/beefy.hpp"
#include "consensus/grandpa/environment.hpp"
#include "consensus/grandpa/has_authority_set_change.hpp"
#include "consensus/timeline/timeline.hpp"
#include "network/peer_manager.hpp"
#include "network/protocols/state_protocol.hpp"
#include "network/protocols/sync_protocol.hpp"
#include "network/types/block_attributes.hpp"
#include "network/warp/protocol.hpp"
#include "primitives/common.hpp"
#include "storage/predefined_keys.hpp"
#include "storage/trie/serialization/trie_serializer.hpp"
#include "storage/trie/trie_batches.hpp"
#include "storage/trie/trie_storage.hpp"
#include "storage/trie_pruner/trie_pruner.hpp"
#include "utils/pool_handler_ready_make.hpp"
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
  constexpr auto kLoadBlocksMaxExpire = std::chrono::seconds{5};

  constexpr auto kRandomWarpInterval = std::chrono::minutes{1};

  kagome::network::BlockAttribute attributesForSync(
      kagome::application::SyncMethod method) {
    using SM = kagome::application::SyncMethod;
    switch (method) {
      case SM::Full:
        return kagome::network::BlocksRequest::kBasicAttributes;
      case SM::Fast:
      case SM::FastWithoutState:
      case SM::Warp:
        return kagome::network::BlockAttribute::HEADER
             | kagome::network::BlockAttribute::JUSTIFICATION;
      case SM::Auto:
        UNREACHABLE;
    }
    return kagome::network::BlocksRequest::kBasicAttributes;
  }
}  // namespace

namespace kagome::network {

  SynchronizerImpl::SynchronizerImpl(
      const application::AppConfiguration &app_config,
      application::AppStateManager &app_state_manager,
      std::shared_ptr<blockchain::BlockTree> block_tree,
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
        block_storage_{std::move(block_storage)} {
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
    return true;
  }
  void SynchronizerImpl::stop() {
    node_is_shutting_down_ = true;
  }

  bool SynchronizerImpl::subscribeToBlock(
      const primitives::BlockInfo &block_info, SyncResultHandler &&handler) {
    // Check if block is already in tree
    if (block_tree_->has(block_info.hash)) {
      scheduler_->schedule(
          [handler = std::move(handler), block_info] { handler(block_info); });
      return false;
    }

    auto last_finalized_block = block_tree_->getLastFinalized();
    // Check if block from discarded side-chain
    if (last_finalized_block.number >= block_info.number) {
      scheduler_->schedule(
          [handler = std::move(handler)] { handler(Error::DISCARDED_BLOCK); });
      return false;
    }

    // Check if block has arrived too early
    auto best_block = block_tree_->bestBlock();
    if (best_block.number + kMaxDistanceToBlockForSubscription
        < block_info.number) {
      scheduler_->schedule([handler = std::move(handler)] {
        handler(Error::ARRIVED_TOO_EARLY);
      });
      return false;
    }

    subscriptions_.emplace(block_info, std::move(handler));
    return true;
  }

  void SynchronizerImpl::notifySubscribers(const primitives::BlockInfo &block,
                                           outcome::result<void> res) {
    auto range = subscriptions_.equal_range(block);
    for (auto it = range.first; it != range.second;) {
      auto cit = it++;
      if (auto node = subscriptions_.extract(cit)) {
        if (res.has_error()) {
          auto error = res.as_failure();
          scheduler_->schedule(
              [handler = std::move(node.mapped()), error] { handler(error); });
        } else {
          scheduler_->schedule(
              [handler = std::move(node.mapped()), block] { handler(block); });
        }
      }
    }
  }

  bool SynchronizerImpl::syncByBlockInfo(
      const primitives::BlockInfo &block_info,
      const libp2p::peer::PeerId &peer_id,
      Synchronizer::SyncResultHandler &&handler,
      bool subscribe_to_block) {
    auto best_block = block_tree_->bestBlock();

    // Provided block is equal our best one. Nothing needs to do.
    if (block_info == best_block) {
      if (handler) {
        handler(block_info);
      }
      return false;
    }

    // Subscribe on demand
    if (subscribe_to_block) {
      subscribeToBlock(block_info, std::move(handler));
    }

    // If provided block is already enqueued, just remember peer
    if (auto it = known_blocks_.find(block_info.hash);
        it != known_blocks_.end()) {
      auto &block_in_queue = it->second;
      block_in_queue.peers.emplace(peer_id);
      if (handler) {
        handler(block_info);
      }
      return false;
    }

    // We are communicating with one peer only for one issue.
    // If peer is already in use, don't start an additional issue.
    auto peer_is_busy = not busy_peers_.emplace(peer_id).second;
    if (peer_is_busy) {
      SL_TRACE(
          log_,
          "Can't syncByBlockHeader block {} is received from {}: Peer busy",
          block_info,
          peer_id);
      return false;
    }
    SL_TRACE(log_, "Peer {} marked as busy", peer_id);

    const auto &last_finalized_block = block_tree_->getLastFinalized();

    // First we need to find the best common block to avoid manipulations with
    // blocks what already exists on node.
    //
    // Find will be doing in interval between definitely known common block and
    // potentially unknown.
    //
    // Best candidate for lower bound is last finalized (it must be known for
    // all synchronized nodes).
    const auto lower = last_finalized_block.number;

    // Best candidate for upper bound is next potentially known block (next for
    // min of provided and our best)
    const auto upper = std::min(block_info.number, best_block.number) + 1;

    // Search starts with potentially known block (min of provided and our best)
    const auto hint = std::min(block_info.number, best_block.number);

    BOOST_ASSERT(lower < upper);

    // Callback what will be called at the end of finding the best common block
    auto find_handler =
        [wp{weak_from_this()}, peer_id, handler = std::move(handler)](
            outcome::result<primitives::BlockInfo> res) mutable {
          if (auto self = wp.lock()) {
            // Remove peer from list of busy peers
            if (self->busy_peers_.erase(peer_id) > 0) {
              SL_TRACE(self->log_, "Peer {} unmarked as busy", peer_id);
            }

            // Finding the best common block was failed
            if (not res.has_value()) {
              if (handler) {
                handler(res.as_failure());
              }
              return;
            }

            // If provided block is already enqueued, just remember peer
            auto &block_info = res.value();
            if (auto it = self->known_blocks_.find(block_info.hash);
                it != self->known_blocks_.end()) {
              auto &block_in_queue = it->second;
              block_in_queue.peers.emplace(peer_id);
              if (handler) {
                handler(block_info);
              }
              return;
            }

            // Start to load blocks since found
            SL_DEBUG(self->log_,
                     "Start to load blocks from {} since block {}",
                     peer_id,
                     block_info);
            self->loadBlocks(peer_id, block_info, std::move(handler));
          }
        };

    // Find the best common block
    SL_DEBUG(log_,
             "Start to find common block with {} in #{}..#{} to catch up",
             peer_id,
             lower,
             upper);
    findCommonBlock(peer_id, lower, upper, hint, std::move(find_handler));
    return true;
  }

  bool SynchronizerImpl::syncByBlockHeader(
      const primitives::BlockHeader &header,
      const libp2p::peer::PeerId &peer_id,
      Synchronizer::SyncResultHandler &&handler) {
    const auto &block_info = header.blockInfo();

    // Block was applied before
    if (block_tree_->has(block_info.hash)) {
      return false;
    }

    // Block is already enqueued
    if (auto it = known_blocks_.find(block_info.hash);
        it != known_blocks_.end()) {
      auto &block_in_queue = it->second;
      block_in_queue.peers.emplace(peer_id);
      return false;
    }

    // Number of provided block header greater currently watched.
    // Reset watched blocks list and start to watch the block with new number
    if (watched_blocks_number_ < header.number) {
      watched_blocks_number_ = header.number;
      watched_blocks_.clear();
    }
    // If number of provided block header is the same of watched, add handler
    // for this block
    if (watched_blocks_number_ == header.number) {
      watched_blocks_.emplace(block_info.hash, std::move(handler));
    }

    // If parent of provided block is in chain, start to load it immediately
    bool parent_is_known =
        known_blocks_.find(header.parent_hash) != known_blocks_.end()
        or block_tree_->has(header.parent_hash);

    if (parent_is_known) {
      loadBlocks(peer_id, block_info, [wp{weak_from_this()}](auto res) {
        if (auto self = wp.lock()) {
          SL_TRACE(self->log_, "Block(s) enqueued to apply by announce");
        }
      });
      return true;
    }

    // Otherwise, is using base way to enqueue
    return syncByBlockInfo(
        block_info,
        peer_id,
        [wp{weak_from_this()}](auto res) {
          if (auto self = wp.lock()) {
            SL_TRACE(self->log_, "Block(s) enqueued to load by announce");
          }
        },
        false);
  }

  void SynchronizerImpl::findCommonBlock(
      const libp2p::peer::PeerId &peer_id,
      primitives::BlockNumber lower,
      primitives::BlockNumber upper,
      primitives::BlockNumber hint,
      SyncResultHandler &&handler,
      std::map<primitives::BlockNumber, primitives::BlockHash> &&observed) {
    network::BlocksRequest request{.fields = network::BlockAttribute::HEADER,
                                   .from = hint,
                                   .direction = network::Direction::ASCENDING,
                                   .max = 1};
    auto response_handler = [wp{weak_from_this()},
                             lower,
                             upper,
                             target = hint,
                             peer_id,
                             handler = std::move(handler),
                             observed = std::move(observed)](
                                auto &&response_res) mutable {
      auto self = wp.lock();
      if (not self) {
        return;
      }

      // Any error interrupts finding common block
      if (response_res.has_error()) {
        SL_VERBOSE(self->log_,
                   "Can't check if block #{} in #{}..#{} is common with {}: {}",
                   target,
                   lower,
                   upper - 1,
                   peer_id,
                   response_res.error());
        handler(response_res.as_failure());
        return;
      }
      auto &blocks = response_res.value().blocks;

      // No block in response is abnormal situation. Requested block must be
      // existed because finding in interval of numbers of blocks that must
      // exist
      if (blocks.empty()) {
        SL_VERBOSE(self->log_,
                   "Can't check if block #{} in #{}..#{} is common with {}: "
                   "Response does not have any blocks",
                   target,
                   lower,
                   upper - 1,
                   peer_id);
        handler(Error::EMPTY_RESPONSE);
        return;
      }

      auto hash = blocks.front().hash;

      observed.emplace(target, hash);

      for (;;) {
        // Check if block is known (is already enqueued or is in block tree)
        bool block_is_known =
            self->known_blocks_.find(hash) != self->known_blocks_.end()
            or self->block_tree_->has(hash);

        // Interval of finding is totally narrowed. Common block should be found
        if (target == lower) {
          if (block_is_known) {
            // Common block is found
            SL_DEBUG(self->log_,
                     "Found best common block with {}: {}",
                     peer_id,
                     BlockInfo(target, hash));
            handler(BlockInfo(target, hash));
            return;
          }

          // Common block is not found. It is abnormal situation. Requested
          // block must be existed because finding in interval of numbers of
          // blocks that must exist
          SL_WARN(self->log_, "Not found any common block with {}", peer_id);
          handler(Error::EMPTY_RESPONSE);
          return;
        }

        // NOLINTNEXTLINE(cppcoreguidelines-init-variables)
        primitives::BlockNumber hint;

        // Narrowing interval for next iteration
        if (block_is_known) {
          SL_TRACE(self->log_,
                   "Block {} of {} is found locally",
                   BlockInfo(target, hash),
                   peer_id);

          // Narrowing interval to continue above
          lower = target;
          hint = lower + (upper - lower) / 2;
        } else {
          SL_TRACE(self->log_,
                   "Block {} of {} is not found locally",
                   BlockInfo(target, hash),
                   peer_id,
                   lower,
                   upper - 1);

          // Step for next iteration
          auto step = upper - target;

          // Narrowing interval to continue below
          upper = target;
          hint = upper - std::min(step, (upper - lower) / 2);
        }
        hint = lower + (upper - lower) / 2;

        // Try again with narrowed interval

        auto it = observed.find(hint);

        // This block number was observed early
        if (it != observed.end()) {
          target = hint;
          hash = it->second;

          SL_TRACE(
              self->log_,
              "Block {} of {} is already observed. Continue without request",
              BlockInfo(target, hash),
              peer_id);
          continue;
        }

        // This block number has not observed yet
        self->findCommonBlock(peer_id,
                              lower,
                              upper,
                              hint,
                              std::move(handler),
                              std::move(observed));
        break;
      }
    };

    SL_TRACE(log_,
             "Check if block #{} in #{}..#{} is common with {}",
             hint,
             lower,
             upper - 1,
             peer_id);
    fetch(peer_id,
          std::move(request),
          "find common block",
          std::move(response_handler));
  }

  void SynchronizerImpl::loadBlocks(const libp2p::peer::PeerId &peer_id,
                                    primitives::BlockInfo from,
                                    SyncResultHandler &&handler) {
    network::BlocksRequest request{
        .fields = attributesForSync(sync_method_),
        .from = from.hash,
        .direction = network::Direction::ASCENDING,
    };

    if (recent_requests_.contains({peer_id, request.fingerprint()})) {
      if (handler) {
        handler(Error::DUPLICATE_REQUEST);
      }
      return;
    }

    auto now = scheduler_->now();
    if (from.number < load_blocks_max_.first
        and now - load_blocks_max_.second < kLoadBlocksMaxExpire
        and not timeline_.get()->wasSynchronized()) {
      if (handler) {
        handler(Error::ALREADY_IN_QUEUE);
      }
      return;
    }

    if (not load_blocks_.emplace(from).second) {
      if (handler) {
        handler(Error::ALREADY_IN_QUEUE);
      }
      return;
    }
    load_blocks_max_ = {from.number, now};

    auto response_handler =
        [wp{weak_from_this()},
         from,
         peer_id,
         handler = std::move(handler),
         need_body = has(request.fields, BlockAttribute::BODY),
         parent_hash = primitives::BlockHash{}](
            outcome::result<BlocksResponse> response_res) mutable {
          auto self = wp.lock();
          if (not self) {
            return;
          }
          self->load_blocks_.erase(from);

          // Any error interrupts loading of blocks
          if (response_res.has_error()) {
            SL_VERBOSE(self->log_,
                       "Can't load blocks from {} beginning block {}: {}",
                       peer_id,
                       from,
                       response_res.error());
            if (handler) {
              handler(response_res.as_failure());
            }
            return;
          }
          auto &blocks = response_res.value().blocks;

          // No block in response is abnormal situation.
          // At least one starting block should be returned as existing
          if (blocks.empty()) {
            SL_VERBOSE(self->log_,
                       "Can't load blocks from {} beginning block {}: "
                       "Response does not have any blocks",
                       peer_id,
                       from);
            if (handler) {
              handler(Error::EMPTY_RESPONSE);
            }
            return;
          }

          SL_TRACE(self->log_,
                   "{} blocks are loaded from {} beginning block {}",
                   blocks.size(),
                   peer_id,
                   from);

          if (blocks[0].header
              and blocks[0].header->number
                      > self->block_tree_->getLastFinalized().number
              and not self->known_blocks_.contains(
                  blocks[0].header->parent_hash)
              and not self->block_tree_->has(blocks[0].header->parent_hash)) {
            if (handler) {
              handler(Error::DISCARDED_BLOCK);
            }
            return;
          }

          bool some_blocks_added = false;
          primitives::BlockInfo last_loaded_block;

          for (auto &block : blocks) {
            // Check if header is provided
            if (not block.header.has_value()) {
              SL_VERBOSE(self->log_,
                         "Can't load blocks from {} starting from block {}: "
                         "Received block without header",
                         peer_id,
                         from);
              if (handler) {
                handler(Error::RESPONSE_WITHOUT_BLOCK_HEADER);
              }
              return;
            }
            // Check if body is provided
            if (need_body and block.header->number != 0
                and not block.body.has_value()) {
              SL_VERBOSE(self->log_,
                         "Can't load blocks from {} starting from block {}: "
                         "Received block without body",
                         peer_id,
                         from);
              if (handler) {
                handler(Error::RESPONSE_WITHOUT_BLOCK_BODY);
              }
              return;
            }
            auto &header = block.header.value();

            const auto &last_finalized_block =
                self->block_tree_->getLastFinalized();

            // Check by number if block is not finalized yet
            if (last_finalized_block.number >= header.number) {
              if (last_finalized_block.number == header.number) {
                if (last_finalized_block.hash != block.hash) {
                  SL_VERBOSE(
                      self->log_,
                      "Can't load blocks from {} starting from block {}: "
                      "Received discarded block {}",
                      peer_id,
                      from,
                      BlockInfo(header.number, block.hash));
                  if (handler) {
                    handler(Error::DISCARDED_BLOCK);
                  }
                  return;
                }

                SL_TRACE(self->log_,
                         "Skip block {} received from {}: "
                         "it is finalized with block #{}",
                         BlockInfo(header.number, block.hash),
                         peer_id,
                         last_finalized_block.number);
                continue;
              }

              SL_TRACE(self->log_,
                       "Skip block {} received from {}: "
                       "it is below the last finalized block #{}",
                       BlockInfo(header.number, block.hash),
                       peer_id,
                       last_finalized_block.number);
              continue;
            }

            // Check if block is not discarded
            if (last_finalized_block.number + 1 == header.number) {
              if (last_finalized_block.hash != header.parent_hash) {
                SL_ERROR(self->log_,
                         "Can't complete blocks loading from {} starting from "
                         "block {}: Received discarded block {}",
                         peer_id,
                         from,
                         BlockInfo(header.number, header.parent_hash));
                if (handler) {
                  handler(Error::DISCARDED_BLOCK);
                }
                return;
              }

              // Start to check parents
              parent_hash = header.parent_hash;
            }

            // Check if block is in chain
            static const primitives::BlockHash zero_hash;
            if (parent_hash != header.parent_hash && parent_hash != zero_hash) {
              SL_ERROR(self->log_,
                       "Can't complete blocks loading from {} starting from "
                       "block {}: Received block is not descendant of previous",
                       peer_id,
                       from);
              if (handler) {
                handler(Error::WRONG_ORDER);
              }
              return;
            }

            // Calculate and save hash, 'cause it's new received block
            primitives::calculateBlockHash(header, *self->hasher_);

            // Check if hash is valid
            if (block.hash != header.hash()) {
              SL_ERROR(self->log_,
                       "Can't complete blocks loading from {} starting from "
                       "block {}: "
                       "Received block whose hash does not match the header",
                       peer_id,
                       from);
              if (handler) {
                handler(Error::INVALID_HASH);
              }
              return;
            }

            last_loaded_block = header.blockInfo();

            parent_hash = block.hash;

            // Add block in queue and save peer or just add peer for existing
            // record
            auto it = self->known_blocks_.find(block.hash);
            if (it == self->known_blocks_.end()) {
              self->known_blocks_.emplace(block.hash,
                                          KnownBlock{
                                              .data = block,
                                              .peers = {peer_id},
                                          });
              self->metric_import_queue_length_->set(
                  self->known_blocks_.size());
            } else {
              it->second.peers.emplace(peer_id);
              SL_TRACE(self->log_,
                       "Skip block {} received from {}: already enqueued",
                       BlockInfo(header.number, block.hash),
                       peer_id);
              continue;
            }

            SL_TRACE(self->log_,
                     "Enqueue block {} received from {}",
                     BlockInfo(header.number, block.hash),
                     peer_id);

            self->generations_.emplace(header.blockInfo());
            self->ancestry_.emplace(header.parent_hash, block.hash);

            some_blocks_added = true;
          }

          SL_TRACE(self->log_, "Block loading is finished");
          if (handler) {
            handler(last_loaded_block);
          }

          if (some_blocks_added) {
            SL_TRACE(self->log_, "Enqueued some new blocks: schedule applying");
            self->scheduler_->schedule([wp] {
              if (auto self = wp.lock()) {
                self->applyNextBlock();
              }
            });
          }
        };

    fetch(peer_id,
          std::move(request),
          "load blocks",
          std::move(response_handler));
  }

  void SynchronizerImpl::syncState(const libp2p::peer::PeerId &peer_id,
                                   const primitives::BlockInfo &block,
                                   SyncResultHandler &&handler) {
    std::unique_lock lock{state_sync_mutex_};
    if (state_sync_) {
      SL_TRACE(log_,
               "State sync request was not sent to {} for block {}: "
               "previous request in progress",
               peer_id,
               block);
      return;
    }
    auto _header = block_tree_->getBlockHeader(block.hash);
    if (not _header) {
      lock.unlock();
      handler(_header.error());
      return;
    }
    auto &header = _header.value();
    if (storage_->getEphemeralBatchAt(header.state_root)) {
      afterStateSync();
      lock.unlock();
      handler(block);
      return;
    }
    if (not state_sync_flow_ or state_sync_flow_->blockInfo() != block) {
      state_sync_flow_.emplace(trie_node_db_, block, header);
    }
    state_sync_.emplace(StateSync{
        .peer = peer_id,
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
      auto ok = self->syncState(lock, std::move(_res));
      if (not ok) {
        auto cb = std::move(self->state_sync_->cb);
        SL_WARN(self->log_, "State syncing failed with error: {}", ok.error());
        self->state_sync_.reset();
        lock.unlock();
        cb(ok.error());
      }
    };

    protocol->request(
        state_sync_->peer, std::move(request), std::move(response_handler));
  }

  outcome::result<void> SynchronizerImpl::syncState(
      std::unique_lock<std::mutex> &lock,
      outcome::result<StateResponse> &&_res) {
    OUTCOME_TRY(res, std::move(_res));
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
    cb(block);
    return outcome::success();
  }

  void SynchronizerImpl::post_block_addition(
      outcome::result<void> block_addition_result,
      Synchronizer::SyncResultHandler &&handler,
      const primitives::BlockHash &hash) {
    REINVOKE(*main_pool_handler_,
             post_block_addition,
             block_addition_result,
             std::move(handler),
             hash);

    processBlockAdditionResult(block_addition_result, hash, std::move(handler));
    ancestry_.erase(hash);
    postApplyBlock();
  }

  void SynchronizerImpl::applyNextBlock() {
    auto any_block_applied = false;
    if (generations_.empty()) {
      return;
    }
    while (not generations_.empty()) {
      auto block_info = *generations_.begin();
      auto pop = [&] { generations_.erase(generations_.begin()); };
      auto it = known_blocks_.find(block_info.hash);
      if (it == known_blocks_.end()) {
        pop();
        continue;
      }
      auto &block_data = it->second.data;
      BOOST_ASSERT(block_data.header.has_value());
      auto parent = block_data.header->parentInfo();
      if (parent and not block_tree_->has(parent->hash)) {
        break;
      }
      pop();
      any_block_applied = true;

      const auto &last_finalized_block = block_tree_->getLastFinalized();

      SyncResultHandler handler;

      if (watched_blocks_number_ == block_data.header->number) {
        if (auto wbn_node = watched_blocks_.extract(block_info.hash)) {
          handler = std::move(wbn_node.mapped());
        }
      }

      // Skip applied and finalized blocks and
      //  discard side-chain below last finalized
      if (block_data.header->number <= last_finalized_block.number) {
        if (not block_tree_->has(block_info.hash)) {
          auto n = discardBlock(block_data.hash);
          SL_WARN(
              log_,
              "Block {} {} not applied as discarded",
              block_info,
              n ? fmt::format("and {} others have", n) : fmt::format("has"));
          if (handler) {
            handler(Error::DISCARDED_BLOCK);
          }
        }

      } else {
        auto callback = [WEAK_SELF, block_info, handler{std::move(handler)}](
                            auto &&block_addition_result) mutable {
          WEAK_LOCK(self);
          self->post_block_addition(std::move(block_addition_result),
                                    std::move(handler),
                                    block_info.hash);
        };

        if (sync_method_ == application::SyncMethod::Full) {
          // Regular syncing
          primitives::Block block{
              .header = std::move(block_data.header.value()),
              .body = std::move(block_data.body.value()),
          };
          block_executor_->applyBlock(
              std::move(block), block_data.justification, std::move(callback));

        } else {
          // Fast syncing
          if (not state_sync_) {
            // Headers loading
            block_appender_->appendHeader(std::move(block_data.header.value()),
                                          block_data.justification,
                                          std::move(callback));

          } else {
            // State syncing in progress; Temporary discard all new blocks
            auto n = discardBlock(block_data.hash);
            SL_WARN(
                log_,
                "Block {} {} not applied as discarded: "
                "state syncing on block in progress",
                block_info,
                n ? fmt::format("and {} others have", n) : fmt::format("has"));
            if (handler) {
              handler(Error::DISCARDED_BLOCK);
            }
            return;
          }
        }
        return;
      }
      ancestry_.erase(block_info.hash);
    }
    if (any_block_applied) {
      postApplyBlock();
    }
  }

  void SynchronizerImpl::processBlockAdditionResult(
      outcome::result<void> block_addition_result,
      const primitives::BlockHash &hash,
      SyncResultHandler &&handler) {
    auto node = known_blocks_.extract(hash);
    if (node) {
      auto &block_data = node.mapped().data;
      BOOST_ASSERT(block_data.header.has_value());
      const BlockInfo block_info(block_data.header->number, block_data.hash);

      notifySubscribers(block_info, block_addition_result);

      if (not block_addition_result.has_value()) {
        if (block_addition_result
            != outcome::failure(blockchain::BlockTreeError::BLOCK_EXISTS)) {
          auto n = discardBlock(block_data.hash);
          SL_WARN(log_,
                  "Block {} {} been discarded: {}",
                  block_info,
                  n ? fmt::format("and {} others have", n) : fmt::format("has"),
                  block_addition_result.error());
          if (handler) {
            std::move(handler)(Error::DISCARDED_BLOCK);
          }
        } else {
          SL_DEBUG(log_, "Block {} is skipped as existing", block_info);
          if (handler) {
            std::move(handler)(block_info);
          }
        }
      } else {
        telemetry_->notifyBlockImported(
            block_info, telemetry::BlockOrigin::kNetworkInitialSync);
        if (handler) {
          std::move(handler)(block_info);
        }

        if (block_data.beefy_justification) {
          beefy_->onJustification(block_data.hash,
                                  std::move(*block_data.beefy_justification));
        }
      }
    }
  }

  void SynchronizerImpl::postApplyBlock() {
    auto minPreloadedBlockAmount = sync_method_ == application::SyncMethod::Full
                                     ? kMinPreloadedBlockAmount
                                     : kMinPreloadedBlockAmountForFastSyncing;

    if (known_blocks_.size() < minPreloadedBlockAmount) {
      SL_TRACE(log_,
               "{} blocks in queue: ask next portion of block",
               known_blocks_.size());
      askNextPortionOfBlocks();
    } else {
      SL_TRACE(log_, "{} blocks in queue", known_blocks_.size());
    }
    metric_import_queue_length_->set(known_blocks_.size());
    scheduler_->schedule([wp{weak_from_this()}] {
      if (auto self = wp.lock()) {
        self->applyNextBlock();
      }
    });
  }

  size_t SynchronizerImpl::discardBlock(
      const primitives::BlockHash &hash_of_discarding_block) {
    std::queue<primitives::BlockHash> queue;
    queue.emplace(hash_of_discarding_block);

    size_t affected = 0;
    while (not queue.empty()) {
      const auto &hash = queue.front();

      if (auto it = known_blocks_.find(hash); it != known_blocks_.end()) {
        auto number = it->second.data.header->number;
        notifySubscribers({number, hash}, Error::DISCARDED_BLOCK);

        known_blocks_.erase(it);
        affected++;
      }

      auto range = ancestry_.equal_range(hash);
      for (auto it = range.first; it != range.second; ++it) {
        queue.emplace(it->second);
      }
      ancestry_.erase(range.first, range.second);

      queue.pop();
    }

    metric_import_queue_length_->set(known_blocks_.size());
    return affected;
  }

  void SynchronizerImpl::prune(const primitives::BlockInfo &finalized_block) {
    // Remove blocks whose numbers less finalized one
    while (not generations_.empty()) {
      auto block_info = *generations_.begin();
      if (block_info.number > finalized_block.number) {
        break;
      }
      if (block_info.number == finalized_block.number) {
        if (block_info.hash != finalized_block.hash) {
          discardBlock(block_info.hash);
        }
        continue;
      }
      generations_.erase(*generations_.begin());
      notifySubscribers(block_info, Error::DISCARDED_BLOCK);
      known_blocks_.erase(block_info.hash);
      ancestry_.erase(block_info.hash);
    }

    metric_import_queue_length_->set(known_blocks_.size());
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

  void SynchronizerImpl::askNextPortionOfBlocks() {
    bool false_val = false;
    if (not asking_blocks_portion_in_progress_.compare_exchange_strong(
            false_val, true)) {
      SL_TRACE(log_, "Asking portion of blocks in progress");
      return;
    }
    SL_TRACE(log_, "Begin asking portion of blocks");

    for (auto g_it = generations_.rbegin(); g_it != generations_.rend();
         ++g_it) {
      auto &block_info = *g_it;
      auto b_it = known_blocks_.find(block_info.hash);
      if (b_it == known_blocks_.end()) {
        SL_TRACE(log_, "Block {} is unknown. Go to next one", block_info);
        continue;
      }

      auto &peers = b_it->second.peers;
      if (peers.empty()) {
        SL_TRACE(
            log_, "Block {} don't have any peer. Go to next one", block_info);
        continue;
      }

      for (auto p_it = peers.begin(); p_it != peers.end();) {
        auto cp_it = p_it++;

        auto &peer_id = *cp_it;

        if (busy_peers_.find(peer_id) != busy_peers_.end()) {
          SL_TRACE(log_, "Peer {} for block {} is busy", peer_id, block_info);
          continue;
        }

        busy_peers_.insert(peers.extract(cp_it));
        SL_TRACE(log_, "Peer {} marked as busy", peer_id);

        auto handler = [wp{weak_from_this()}, peer_id](const auto &res) {
          if (auto self = wp.lock()) {
            if (self->busy_peers_.erase(peer_id) > 0) {
              SL_TRACE(self->log_, "Peer {} unmarked as busy", peer_id);
            }
            SL_TRACE(self->log_, "End asking portion of blocks");
            self->asking_blocks_portion_in_progress_ = false;
            if (not res.has_value()) {
              SL_DEBUG(self->log_,
                       "Loading next portion of blocks from {} is failed: {}",
                       peer_id,
                       res.error());
              return;
            }
            SL_DEBUG(self->log_,
                     "Portion of blocks from {} is loaded till {}",
                     peer_id,
                     res.value());
            if (self->known_blocks_.empty()) {
              self->askNextPortionOfBlocks();
            }
          }
        };

        if (sync_method_ == application::SyncMethod::Full) {
          auto lower = generations_.begin()->number;
          auto upper = generations_.rbegin()->number + 1;
          auto hint = generations_.rbegin()->number;

          SL_DEBUG(
              log_,
              "Start to find common block with {} in #{}..#{} to fill queue",
              peer_id,
              generations_.begin()->number,
              generations_.rbegin()->number);
          findCommonBlock(
              peer_id,
              lower,
              upper,
              hint,
              [wp{weak_from_this()}, peer_id, handler = std::move(handler)](
                  outcome::result<primitives::BlockInfo> res) {
                if (auto self = wp.lock()) {
                  if (not res.has_value()) {
                    SL_DEBUG(self->log_,
                             "Can't load next portion of blocks from {}: {}",
                             peer_id,
                             res.error());
                    handler(res);
                    return;
                  }
                  auto &common_block_info = res.value();
                  SL_DEBUG(self->log_,
                           "Start to load next portion of blocks from {} "
                           "since block {}",
                           peer_id,
                           common_block_info);
                  self->loadBlocks(
                      peer_id, common_block_info, std::move(handler));
                }
              });
        } else {
          SL_DEBUG(log_,
                   "Start to load next portion of blocks from {} "
                   "since block {}",
                   peer_id,
                   block_info);
          loadBlocks(peer_id, block_info, std::move(handler));
        }
        return;
      }

      SL_TRACE(log_,
               "Block {} doesn't have appropriate peer. Go to next one",
               block_info);
    }

    SL_TRACE(log_, "End asking portion of blocks: none");
    asking_blocks_portion_in_progress_ = false;
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
                peer{*chosen}](outcome::result<BlocksResponse> r) mutable {
      auto self = weak.lock();
      if (not self) {
        return;
      }
      self->busy_peers_.erase(peer);
      if (not r) {
        return cb(r.error());
      }
      auto &blocks = r.value().blocks;
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
                   outcome::result<WarpResponse> r) mutable {
      auto self = weak.lock();
      if (not self) {
        return;
      }
      self->busy_peers_.erase(peer);
      if (not r) {
        return cb(r.error());
      }
      auto &blocks = r.value().proofs;
      for (const auto &block : blocks) {
        self->grandpa_environment_->applyJustification(
            block.justification.block_info,
            {scale::encode(block.justification).value()},
            [cb{std::move(cb)}](outcome::result<void> r) {
              if (not r) {
                cb(r.error());
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
    generations_.clear();
    ancestry_.clear();
    recent_requests_.clear();
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

    auto cb2 = [weak{weak_from_this()},
                expected,
                isFinalized,
                cb{std::move(cb)},
                peer{*chosen}](outcome::result<BlocksResponse> r) mutable {
      auto self = weak.lock();
      if (not self) {
        return cb(Error::SHUTTING_DOWN);
      }

      self->busy_peers_.erase(peer);
      if (not r) {
        return cb(r.error());
      }

      auto &blocks = r.value().blocks;
      if (blocks.empty()) {
        return cb(Error::EMPTY_RESPONSE);
      }
      for (auto &b : blocks) {
        auto &header = b.header;

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

        if (auto er = self->block_storage_->putBlockHeader(headerValue);
            er.has_error()) {
          SL_ERROR(self->log_, "Failed to put block header: {}", er.error());
          return cb(er.error());
        }

        if (isFinalized) {
          if (auto er = self->block_storage_->assignNumberToHash(headerInfo);
              er.has_error()) {
            SL_ERROR(
                self->log_, "Failed to assign number to hash: {}", er.error());
            return cb(er.error());
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

  void SynchronizerImpl::randomWarp() {
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
    scheduler_->schedule(
        [WEAK_SELF] {
          WEAK_LOCK(self);
          self->randomWarp();
        },
        kRandomWarpInterval);
  }
}  // namespace kagome::network
