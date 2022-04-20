/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/synchronizer_impl.hpp"

#include <random>

#include "application/app_configuration.hpp"
#include "blockchain/block_tree_error.hpp"
#include "network/helpers/peer_id_formatter.hpp"
#include "primitives/common.hpp"
#include "storage/trie/serialization/trie_serializer.hpp"
#include "storage/trie/trie_storage.hpp"

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

  kagome::network::BlockAttributes attributesForSync(
      kagome::application::AppConfiguration::SyncMethod method) {
    using SM = kagome::application::AppConfiguration::SyncMethod;
    switch (method) {
      case SM::Full:
        return kagome::network::BlocksRequest::kBasicAttributes;
      case SM::Fast:
        return kagome::network::BlockAttribute::HEADER;
    }
    return kagome::network::BlocksRequest::kBasicAttributes;
  }
}  // namespace

namespace kagome::network {

  SynchronizerImpl::SynchronizerImpl(
      const application::AppConfiguration &app_config,
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<consensus::BlockAppender> block_appender,
      std::shared_ptr<consensus::BlockExecutor> block_executor,
      std::shared_ptr<storage::trie::TrieSerializer> serializer,
      std::shared_ptr<storage::trie::TrieStorage> storage,
      std::shared_ptr<network::Router> router,
      std::shared_ptr<libp2p::basic::Scheduler> scheduler,
      std::shared_ptr<crypto::Hasher> hasher)
      : block_tree_(std::move(block_tree)),
        block_appender_(std::move(block_appender)),
        block_executor_(std::move(block_executor)),
        serializer_(std::move(serializer)),
        storage_(std::move(storage)),
        router_(std::move(router)),
        scheduler_(std::move(scheduler)),
        hasher_(std::move(hasher)) {
    BOOST_ASSERT(block_tree_);
    BOOST_ASSERT(block_executor_);
    BOOST_ASSERT(serializer_);
    BOOST_ASSERT(storage_);
    BOOST_ASSERT(router_);
    BOOST_ASSERT(scheduler_);
    BOOST_ASSERT(hasher_);

    BOOST_ASSERT(app_state_manager);

    sync_method_ = app_config.syncMethod();

    // Register metrics
    metrics_registry_->registerGaugeFamily(
        kImportQueueLength, "Number of blocks submitted to the import queue");
    metric_import_queue_length_ =
        metrics_registry_->registerGaugeMetric(kImportQueueLength);
    metric_import_queue_length_->set(0);

    app_state_manager->atShutdown([this] { node_is_shutting_down_ = true; });
  }

  bool SynchronizerImpl::subscribeToBlock(
      const primitives::BlockInfo &block_info, SyncResultHandler &&handler) {
    // Check if block is already in tree
    if (block_tree_->hasBlockHeader(block_info.hash)) {
      scheduler_->schedule(
          [handler = std::move(handler), block_info] { handler(block_info); });
      return false;
    }

    auto last_finalized_block = block_tree_->getLastFinalized();
    // Check if block from discarded side-chain
    if (last_finalized_block.number <= block_info.number) {
      scheduler_->schedule(
          [handler = std::move(handler)] { handler(Error::DISCARDED_BLOCK); });
      return false;
    }

    // Check if block has arrived too early
    auto best_block_res =
        block_tree_->getBestContaining(last_finalized_block.hash, std::nullopt);
    BOOST_ASSERT(best_block_res.has_value());
    const auto &best_block = best_block_res.value();
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
    // Subscribe on demand
    if (subscribe_to_block) {
      subscribeToBlock(block_info, std::move(handler));
    }

    // If provided block is already enqueued, just remember peer
    if (auto it = known_blocks_.find(block_info.hash);
        it != known_blocks_.end()) {
      auto &block_in_queue = it->second;
      block_in_queue.peers.emplace(peer_id);
      if (handler) handler(block_info);
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

    auto best_block_res =
        block_tree_->getBestContaining(last_finalized_block.hash, std::nullopt);
    BOOST_ASSERT(best_block_res.has_value());
    const auto &best_block = best_block_res.value();

    // Provided block is equal our best one. Nothing needs to do.
    if (block_info == best_block) {
      if (handler) handler(block_info);
      return false;
    }

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
    Synchronizer::SyncResultHandler find_handler =
        [wp = weak_from_this(), peer_id, handler = std::move(handler)](
            outcome::result<primitives::BlockInfo> res) mutable {
          if (auto self = wp.lock()) {
            // Remove peer from list of busy peers
            if (self->busy_peers_.erase(peer_id) > 0) {
              SL_TRACE(self->log_, "Peer {} unmarked as busy", peer_id);
            }

            // Finding the best common block was failed
            if (not res.has_value()) {
              if (handler) handler(res.as_failure());
              return;
            }

            // If provided block is already enqueued, just remember peer
            auto &block_info = res.value();
            if (auto it = self->known_blocks_.find(block_info.hash);
                it != self->known_blocks_.end()) {
              auto &block_in_queue = it->second;
              block_in_queue.peers.emplace(peer_id);
              if (handler) handler(std::move(block_info));
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
    auto block_hash = hasher_->blake2b_256(scale::encode(header).value());
    const primitives::BlockInfo block_info(header.number, block_hash);

    // Block was applied before
    if (block_tree_->getBlockHeader(block_hash).has_value()) {
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
      watched_blocks_.emplace(block_hash, std::move(handler));
    }

    // If parent of provided block is in chain, start to load it immediately
    bool parent_is_known =
        known_blocks_.find(header.parent_hash) != known_blocks_.end()
        or block_tree_->getBlockHeader(header.parent_hash).has_value();

    if (parent_is_known) {
      loadBlocks(peer_id, block_info, [wp = weak_from_this()](auto res) {
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
        [wp = weak_from_this()](auto res) {
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
    // Interrupts process if node is shutting down
    if (node_is_shutting_down_) {
      handler(Error::SHUTTING_DOWN);
      return;
    }

    network::BlocksRequest request{network::BlockAttribute::HEADER,
                                   hint,
                                   std::nullopt,
                                   network::Direction::ASCENDING,
                                   1};

    auto request_fingerprint = request.fingerprint();

    if (not recent_requests_.emplace(peer_id, request_fingerprint).second) {
      SL_VERBOSE(
          log_,
          "Can't check if block #{} in #{}..#{} is common with {}: {}",
          hint,
          lower,
          upper - 1,
          peer_id,
          outcome::result<void>(Error::DUPLICATE_REQUEST).error().message());
      handler(Error::DUPLICATE_REQUEST);
      return;
    }

    scheduler_->schedule(
        [wp = weak_from_this(), peer_id, request_fingerprint] {
          if (auto self = wp.lock()) {
            self->recent_requests_.erase(
                std::tuple(peer_id, request_fingerprint));
          }
        },
        kRecentnessDuration);

    auto response_handler = [wp = weak_from_this(),
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
                   response_res.error().message());
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
            or self->block_tree_->getBlockHeader(hash).has_value();

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

    auto protocol = router_->getSyncProtocol();
    BOOST_ASSERT_MSG(protocol, "Router did not provide sync protocol");
    protocol->request(peer_id, std::move(request), std::move(response_handler));
  }

  void SynchronizerImpl::loadBlocks(const libp2p::peer::PeerId &peer_id,
                                    primitives::BlockInfo from,
                                    SyncResultHandler &&handler) {
    // Interrupts process if node is shutting down
    if (node_is_shutting_down_) {
      if (handler) handler(Error::SHUTTING_DOWN);
      return;
    }

    network::BlocksRequest request{attributesForSync(sync_method_),
                                   from.hash,
                                   std::nullopt,
                                   network::Direction::ASCENDING,
                                   std::nullopt};

    auto request_fingerprint = request.fingerprint();

    if (not recent_requests_.emplace(peer_id, request_fingerprint).second) {
      SL_ERROR(
          log_,
          "Can't load blocks from {} beginning block {}: {}",
          peer_id,
          from,
          outcome::result<void>(Error::DUPLICATE_REQUEST).error().message());
      if (handler) handler(Error::DUPLICATE_REQUEST);
      return;
    }

    scheduler_->schedule(
        [wp = weak_from_this(), peer_id, request_fingerprint] {
          if (auto self = wp.lock()) {
            self->recent_requests_.erase(
                std::tuple(peer_id, request_fingerprint));
          }
        },
        kRecentnessDuration);

    auto response_handler = [wp = weak_from_this(),
                             from,
                             peer_id,
                             handler = std::move(handler),
                             parent_hash = primitives::BlockHash{}](
                                auto &&response_res) mutable {
      auto self = wp.lock();
      if (not self) {
        return;
      }

      // Any error interrupts loading of blocks
      if (response_res.has_error()) {
        SL_ERROR(self->log_,
                 "Can't load blocks from {} beginning block {}: {}",
                 peer_id,
                 from,
                 response_res.error().message());
        if (handler) handler(response_res.as_failure());
        return;
      }
      auto &blocks = response_res.value().blocks;

      // No block in response is abnormal situation.
      // At least one starting block should be returned as existing
      if (blocks.empty()) {
        SL_ERROR(self->log_,
                 "Can't load blocks from {} beginning block {}: "
                 "Response does not have any blocks",
                 peer_id,
                 from);
        if (handler) handler(Error::EMPTY_RESPONSE);
        return;
      }

      SL_TRACE(self->log_,
               "{} blocks are loaded from {} beginning block {}",
               blocks.size(),
               peer_id,
               from);

      bool some_blocks_added = false;
      primitives::BlockInfo last_loaded_block;

      for (auto &block : blocks) {
        // Check if header is provided
        if (not block.header.has_value()) {
          SL_ERROR(self->log_,
                   "Can't load blocks from {} starting from block {}: "
                   "Received block without header",
                   peer_id,
                   from);
          if (handler) handler(Error::RESPONSE_WITHOUT_BLOCK_HEADER);
          return;
        }
        // Check if body is provided
        if (not block.header.has_value()) {
          SL_ERROR(self->log_,
                   "Can't load blocks from {} starting from block {}: "
                   "Received block without body",
                   peer_id,
                   from);
          if (handler) handler(Error::RESPONSE_WITHOUT_BLOCK_BODY);
          return;
        }
        auto &header = block.header.value();

        const auto &last_finalized_block =
            self->block_tree_->getLastFinalized();

        // Check by number if block is not finalized yet
        if (last_finalized_block.number >= header.number) {
          if (last_finalized_block.number == header.number) {
            if (last_finalized_block.hash != block.hash) {
              SL_ERROR(self->log_,
                       "Can't load blocks from {} starting from block {}: "
                       "Received discarded block {}",
                       peer_id,
                       from,
                       BlockInfo(header.number, block.hash));
              if (handler) handler(Error::DISCARDED_BLOCK);
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
            if (handler) handler(Error::DISCARDED_BLOCK);
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
          if (handler) handler(Error::WRONG_ORDER);
          return;
        }

        // Check if hash is valid
        auto calculated_hash =
            self->hasher_->blake2b_256(scale::encode(header).value());
        if (block.hash != calculated_hash) {
          SL_ERROR(self->log_,
                   "Can't complete blocks loading from {} starting from "
                   "block {}: "
                   "Received block whose hash does not match the header",
                   peer_id,
                   from);
          if (handler) handler(Error::INVALID_HASH);
          return;
        }

        last_loaded_block = {header.number, block.hash};

        parent_hash = block.hash;

        // Add block in queue and save peer or just add peer for existing record
        auto it = self->known_blocks_.find(block.hash);
        if (it == self->known_blocks_.end()) {
          self->known_blocks_.emplace(block.hash, KnownBlock{block, {peer_id}});
          self->metric_import_queue_length_->set(self->known_blocks_.size());
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

        self->generations_.emplace(header.number, block.hash);
        self->ancestry_.emplace(header.parent_hash, block.hash);

        some_blocks_added = true;
      }

      SL_TRACE(self->log_, "Block loading is finished");
      self->syncState(
          peer_id, last_loaded_block, common::Buffer(), std::move(handler));

      if (some_blocks_added) {
        SL_TRACE(self->log_, "Enqueued some new blocks: schedule applying");
        self->scheduler_->schedule([wp] {
          if (auto self = wp.lock()) {
            self->applyNextBlock();
          }
        });
      }
    };

    auto protocol = router_->getSyncProtocol();
    BOOST_ASSERT_MSG(protocol, "Router did not provide sync protocol");
    protocol->request(peer_id, std::move(request), std::move(response_handler));
  }

  void SynchronizerImpl::syncState(const libp2p::peer::PeerId &peer_id,
                                   primitives::BlockInfo block,
                                   common::Buffer &&key,
                                   SyncResultHandler &&handler) {
    if (sync_method_ == application::AppConfiguration::SyncMethod::Fast && not state_syncing_.load()) {
      state_syncing_.store(true);
      network::StateRequest request{block.hash, {key}, true};

      auto protocol = router_->getStateProtocol();
      BOOST_ASSERT_MSG(protocol, "Router did not provide state protocol");

      SL_TRACE(log_, "State syncing started.");

      auto response_handler = [wp = weak_from_this(),
                               block,
                               peer_id,
                               handler = std::move(handler)](
                                  auto &&response_res) mutable {
        auto self = wp.lock();
        if (not self) {
          return;
        }

        if (response_res.has_error()) {
          SL_WARN(self->log_,
                  "State syncing failed with error: {}",
                  response_res.error().message());
          if (handler) handler(response_res.as_failure());
          return;
        }

        auto batch =
            self->storage_
                ->getPersistentBatchAt(self->serializer_->getEmptyRootHash())
                .value();
        for (const auto &entry : response_res.value().entries[0].entries) {
          std::ignore = batch->put(entry.key, entry.value);
        }
        auto res = batch->commit();
        SL_TRACE(self->log_,
                 "State syncing finished. Root hash: {}",
                 res.value().toHex());
        self->sync_method_ = application::AppConfiguration::SyncMethod::Full;
        if(handler) {
          handler(block);
          self->state_syncing_.store(false);
        }
      };

      protocol->request(
          peer_id, std::move(request), std::move(response_handler));
    } else {
      if (handler) {
        handler(block);
      }
    }
  }

  void SynchronizerImpl::applyNextBlock() {
    if (generations_.empty()) {
      SL_TRACE(log_, "No block for applying");
      return;
    }

    bool false_val = false;
    if (not applying_in_progress_.compare_exchange_strong(false_val, true)) {
      SL_TRACE(log_, "Applying in progress");
      return;
    }
    SL_TRACE(log_, "Begin applying");
    auto cleanup = gsl::finally([this] {
      SL_TRACE(log_, "End applying");
      applying_in_progress_ = false;
    });

    primitives::BlockHash hash;

    while (true) {
      auto generation_node = generations_.extract(generations_.begin());
      if (generation_node) {
        hash = generation_node.mapped();
        break;
      }
      if (generations_.empty()) {
        SL_TRACE(log_, "No block for applying");
        return;
      }
    }

    auto node = known_blocks_.extract(hash);
    if (node) {
      auto &block = node.mapped().data;
      BOOST_ASSERT(block.header.has_value());
      auto number = block.header->number;

      const auto &last_finalized_block = block_tree_->getLastFinalized();

      SyncResultHandler handler;

      if (watched_blocks_number_ == block.header->number) {
        if (auto wbn_node = watched_blocks_.extract(hash)) {
          handler = std::move(wbn_node.mapped());
        }
      }

      if (block.header->number <= last_finalized_block.number) {
        auto header_res = block_tree_->getBlockHeader(hash);
        if (not header_res.has_value()) {
          auto n = discardBlock(block.hash);
          SL_WARN(
              log_,
              "Block {} {} not applied as discarded",
              BlockInfo(number, hash),
              n ? fmt::format("and {} others have", n) : fmt::format("has"));
          if (handler) handler(Error::DISCARDED_BLOCK);
        }
      } else {
        const BlockInfo block_info(block.header->number, block.hash);

        auto applying_res =
            sync_method_ == application::AppConfiguration::SyncMethod::Full
                ? block_executor_->applyBlock(std::move(block))
                : block_appender_->appendBlock(std::move(block));

        notifySubscribers({number, hash}, applying_res);

        if (not applying_res.has_value()) {
          if (applying_res
              != outcome::failure(blockchain::BlockTreeError::BLOCK_EXISTS)) {
            notifySubscribers({number, hash}, applying_res.as_failure());
            auto n = discardBlock(block_info.hash);
            SL_WARN(
                log_,
                "Block {} {} been discarded: {}",
                block_info,
                n ? fmt::format("and {} others have", n) : fmt::format("has"),
                applying_res.error().message());
            if (handler) handler(Error::DISCARDED_BLOCK);
          } else {
            SL_DEBUG(log_, "Block {} is skipped as existing", block_info);
            if (handler) handler(block_info);
          }
        } else {
          telemetry_->notifyBlockImported(
              block_info, telemetry::BlockOrigin::kNetworkInitialSync);
          if (handler) handler(block_info);
        }
      }
    }
    ancestry_.erase(hash);

    if (known_blocks_.size() < kMinPreloadedBlockAmount) {
      SL_TRACE(log_,
               "{} blocks in queue: ask next portion of block",
               known_blocks_.size());
      askNextPortionOfBlocks();
    } else {
      SL_TRACE(log_, "{} blocks in queue", known_blocks_.size());
    }
    metric_import_queue_length_->set(known_blocks_.size());

    scheduler_->schedule([wp = weak_from_this()] {
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
      auto generation_node = generations_.extract(generations_.begin());
      if (generation_node) {
        const auto &number = generation_node.key();
        if (number >= finalized_block.number) {
          break;
        }
        const auto &hash = generation_node.mapped();
        notifySubscribers({number, hash}, Error::DISCARDED_BLOCK);

        known_blocks_.erase(hash);
        ancestry_.erase(hash);
      }
    }

    // Remove blocks whose numbers equal finalized one, excluding finalized one
    auto range = generations_.equal_range(finalized_block.number);
    for (auto it = range.first; it != range.second;) {
      auto cit = it++;
      const auto &hash = cit->second;
      if (hash != finalized_block.hash) {
        discardBlock(hash);
      }
    }

    metric_import_queue_length_->set(known_blocks_.size());
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
      const auto &hash = g_it->second;

      auto b_it = known_blocks_.find(hash);
      if (b_it == known_blocks_.end()) {
        SL_TRACE(log_,
                 "Block {} is unknown. Go to next one",
                 primitives::BlockInfo(g_it->first, hash));
        continue;
      }

      auto &peers = b_it->second.peers;
      if (peers.empty()) {
        SL_TRACE(log_,
                 "Block {} don't have any peer. Go to next one",
                 primitives::BlockInfo(g_it->first, hash));
        continue;
      }

      for (auto p_it = peers.begin(); p_it != peers.end();) {
        auto cp_it = p_it++;

        auto peer_id = *cp_it;

        if (busy_peers_.find(peer_id) != busy_peers_.end()) {
          SL_TRACE(log_,
                   "Peer {} for block {} is busy",
                   peer_id,
                   primitives::BlockInfo(g_it->first, hash));
          continue;
        }

        busy_peers_.insert(peers.extract(cp_it));
        SL_TRACE(log_, "Peer {} marked as busy", peer_id);

        auto handler = [wp = weak_from_this(), peer_id](const auto &res) {
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
                       res.error().message());
              return;
            }
            SL_DEBUG(
                self->log_, "Portion of blocks from {} is loaded", peer_id);
          }
        };

        auto lower = generations_.begin()->first;
        auto upper = generations_.rbegin()->first + 1;
        auto hint = generations_.rbegin()->first;

        SL_DEBUG(log_,
                 "Start to find common block with {} in #{}..#{} to fill queue",
                 peer_id,
                 generations_.begin()->first,
                 generations_.rbegin()->first);
        findCommonBlock(
            peer_id,
            lower,
            upper,
            hint,
            [wp = weak_from_this(), peer_id, handler = std::move(handler)](
                outcome::result<primitives::BlockInfo> res) {
              if (auto self = wp.lock()) {
                if (not res.has_value()) {
                  SL_DEBUG(self->log_,
                           "Can't load next portion of blocks from {}: {}",
                           peer_id,
                           res.error().message());
                  handler(res);
                  return;
                }
                auto &block_info = res.value();
                SL_DEBUG(self->log_,
                         "Start to load next portion of blocks from {} "
                         "since block {}",
                         peer_id,
                         block_info);
                self->loadBlocks(peer_id, block_info, std::move(handler));
              }
            });
        return;
      }

      SL_TRACE(log_,
               "Block {} doesn't have appropriate peer. Go to next one",
               primitives::BlockInfo(g_it->first, hash));
    }

    SL_TRACE(log_, "End asking portion of blocks: none");
    asking_blocks_portion_in_progress_ = false;
  }

}  // namespace kagome::network
