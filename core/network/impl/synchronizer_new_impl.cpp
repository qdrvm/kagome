/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "network/impl/synchronizer_new_impl.hpp"

#include <random>

#include "application/app_configuration.hpp"
#include "blockchain/block_tree_error.hpp"
#include "network/helpers/peer_id_formatter.hpp"
#include "network/types/block_attributes.hpp"
#include "primitives/common.hpp"
#include "storage/changes_trie/changes_tracker.hpp"
#include "storage/trie/serialization/trie_serializer.hpp"
#include "storage/trie/trie_batches.hpp"
#include "storage/trie/trie_storage.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::network, SynchronizerNewImpl::Error, e) {
  using E = kagome::network::SynchronizerNewImpl::Error;
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
        return kagome::network::BlockAttribute::HEADER
               | kagome::network::BlockAttribute::JUSTIFICATION;
    }
    return kagome::network::BlocksRequest::kBasicAttributes;
  }
}  // namespace

namespace kagome::network {

  SynchronizerNewImpl::SynchronizerNewImpl(
      const application::AppConfiguration &app_config,
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<storage::changes_trie::ChangesTracker> changes_tracker,
      std::shared_ptr<consensus::BlockAppender> block_appender,
      std::shared_ptr<consensus::BlockExecutor> block_executor,
      std::shared_ptr<storage::trie::TrieSerializer> serializer,
      std::shared_ptr<storage::trie::TrieStorage> storage,
      std::shared_ptr<network::Router> router,
      std::shared_ptr<libp2p::basic::Scheduler> scheduler,
      std::shared_ptr<crypto::Hasher> hasher)
      : block_tree_(std::move(block_tree)),
        trie_changes_tracker_(std::move(changes_tracker)),
        block_appender_(std::move(block_appender)),
        block_executor_(std::move(block_executor)),
        serializer_(std::move(serializer)),
        storage_(std::move(storage)),
        router_(std::move(router)),
        scheduler_(std::move(scheduler)),
        hasher_(std::move(hasher)) {
    BOOST_ASSERT(block_tree_);
    BOOST_ASSERT(trie_changes_tracker_ != nullptr);
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

  bool SynchronizerNewImpl::subscribeToBlock(
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

  void SynchronizerNewImpl::notifySubscribers(
      const primitives::BlockInfo &block, outcome::result<void> res) {
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

  bool SynchronizerNewImpl::syncByBlockInfo(
      const primitives::BlockInfo &block_info,
      const libp2p::peer::PeerId &peer_id,
      Synchronizer::SyncResultHandler &&handler,
      bool subscribe_to_block) {
    if (not state_syncing_.load() && not syncing_.load()) {
      syncing_.store(true);
    } else {
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
      if (handler) handler(block_info);
      return false;
    }

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

    loadBlocks(peer_id, last_finalized_block, std::move(handler));

    return true;
  }

  bool SynchronizerNewImpl::syncByBlockHeader(
      const primitives::BlockHeader &header,
      const libp2p::peer::PeerId &peer_id,
      Synchronizer::SyncResultHandler &&handler) {
    if (not state_syncing_.load() && not syncing_.load()) {
      syncing_.store(true);
    } else {
      return false;
    }
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

    loadBlocks(peer_id, block_info, std::move(handler));

    return true;
  }

  void SynchronizerNewImpl::loadBlocks(const libp2p::peer::PeerId &peer_id,
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
      }

      if (from.number + 20 >= last_loaded_block.number || blocks.size() < 127) {
        self->applyNextBlock();
        handler(last_loaded_block);
        return;
      }
      self->scheduler_->schedule([self,
                                  peer_id,
                                  handler = std::move(handler),
                                  last_loaded_block]() mutable {
        self->applyNextBlock();
        self->loadBlocks(peer_id, last_loaded_block, std::move(handler));
      });
    };

    auto protocol = router_->getSyncProtocol();
    BOOST_ASSERT_MSG(protocol, "Router did not provide sync protocol");
    protocol->request(peer_id, std::move(request), std::move(response_handler));
  }

  void SynchronizerNewImpl::syncState(const libp2p::peer::PeerId &peer_id,
                                      const primitives::BlockInfo &block,
                                      const std::vector<common::Buffer> &keys,
                                      SyncResultHandler &&handler) {
    if (sync_method_ == application::AppConfiguration::SyncMethod::Fast) {
      // execute if not already started or iteration continues
      if (not state_syncing_.load()
          || (not keys.empty() && not keys[0].empty())) {
        state_syncing_.store(true);
        if (keys[0].empty()) {
          sync_block_ = block;
        }
        network::StateRequest request{block.hash, keys, true};

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

          for (unsigned i = 0; i < response_res.value().entries.size(); ++i) {
            const auto &response = response_res.value().entries[i];

            // get or create batch
            auto batch =
                self->batches_store_.count(response.state_root)
                    ? std::get<2>(self->batches_store_[response.state_root])
                    : self->storage_
                          ->getPersistentBatchAt(
                              self->serializer_->getEmptyRootHash())
                          .value();

            // main storage entries size empty at child storage state syncing
            if (response.entries.size()) {
              SL_TRACE(self->log_,
                       "Syncing {}th item. Current key {}. Keys received {}.",
                       i,
                       response.entries[0].key.toHex(),
                       response.entries.size());
              for (const auto &entry : response.entries) {
                std::ignore = batch->put(entry.key, entry.value);
              }

              // store batch to continue at next response
              if (!response.complete) {
                self->batches_store_[response.state_root] = {
                    response.entries.back().key, i, batch};
              } else {
                self->batches_store_.erase(response.state_root);
              }
            }

            if (response.complete) {
              auto res = batch->commit();
              SL_INFO(
                  self->log_,
                  "{} syncing finished. Root hash: {}. {}.",
                  i ? "Child state" : "State",
                  res.value().toHex(),
                  res.value() == response.state_root ? "Match" : "Don't match");
              if (res.value() != response.state_root) {
                SL_INFO(
                    self->log_, "Should be {}", response.state_root.toHex());
              }
              self->trie_changes_tracker_->onBlockAdded(block.hash);
            }

            // just calculate state entries in main storage for trace log
            if (!i) {
              self->entries_ += response.entries.size();
            }
          }

          // not well formed way to place 0th batch key to front
          std::map<unsigned, common::Buffer> keymap;
          for (const auto &[_, val] : self->batches_store_) {
            unsigned i = std::get<1>(val);
            keymap[i] = std::get<0>(val);
            SL_TRACE(self->log_, "Index: {}, Key: {}", i, keymap[i]);
          }

          std::vector<common::Buffer> keys;
          for (const auto &[_, val] : keymap) {
            keys.push_back(val);
          }

          if (response_res.value().entries[0].complete) {
            self->sync_method_ =
                application::AppConfiguration::SyncMethod::Full;
            if (handler) {
              handler(block);
              self->state_syncing_.store(false);
            }
          } else {
            SL_TRACE(self->log_,
                     "State syncing continues. {} entries loaded",
                     self->entries_);
            self->syncState(peer_id, block, keys, std::move(handler));
          }
        };

        protocol->request(
            peer_id, std::move(request), std::move(response_handler));
      }
    } else {
      if (handler) {
        handler(block);
      }
    }
  }

  void SynchronizerNewImpl::applyNextBlock() {
    if (generations_.empty()) {
      SL_TRACE(log_, "No block for applying");
      return;
    }

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

      if (block.header->number <= last_finalized_block.number) {
        auto header_res = block_tree_->getBlockHeader(hash);
        if (not header_res.has_value()) {
          auto n = discardBlock(block.hash);
          SL_WARN(
              log_,
              "Block {} {} not applied as discarded",
              BlockInfo(number, hash),
              n ? fmt::format("and {} others have", n) : fmt::format("has"));
        }
      } else {
        const BlockInfo block_info(block.header->number, block.hash);

        if (sync_method_ == application::AppConfiguration::SyncMethod::Full
            && sync_block_ && block_info.number <= sync_block_.value().number) {
          SL_WARN(
              log_, "Skip {} till fast synchronized block", block_info.number);
          applyNextBlock();
        } else {
          auto applying_res =
              sync_method_ == application::AppConfiguration::SyncMethod::Full
                  ? block_executor_->applyBlock(std::move(block))
                  : block_appender_->appendBlock(std::move(block));

          if (sync_method_ == application::AppConfiguration::SyncMethod::Full
              && sync_block_
              && block_info.number == sync_block_.value().number + 1) {
            sync_block_ = std::nullopt;
          }

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
            } else {
              SL_DEBUG(log_, "Block {} is skipped as existing", block_info);
              applyNextBlock();
            }
          } else {
            applyNextBlock();
          }
        }
      }
    }
    ancestry_.erase(hash);

    metric_import_queue_length_->set(known_blocks_.size());
  }

  size_t SynchronizerNewImpl::discardBlock(
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

}  // namespace kagome::network
