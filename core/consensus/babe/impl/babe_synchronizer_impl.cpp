/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "babe_synchronizer_impl.hpp"
#include <blockchain/block_tree_error.hpp>
#include <random>

#include "network/types/block_attributes.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(kagome::consensus, BabeSynchronizerImpl::Error, e) {
  using E = kagome::consensus::BabeSynchronizerImpl::Error;
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
      return "Wrong order";
    case E::INVALID_HASH:
      return "Hash does not match";
    case E::ALREADY_IN_QUEUE:
      return "Block is already enqueued";
    case E::PEER_BUSY:
      return "Peer is busy";
  }
  return "unknown error";
}

namespace kagome::consensus {

  BabeSynchronizerImpl::BabeSynchronizerImpl(
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<BlockExecutor> block_executor,
      std::shared_ptr<network::Router> router,
      std::shared_ptr<libp2p::basic::Scheduler> scheduler,
      std::shared_ptr<crypto::Hasher> hasher)
      : block_tree_(std::move(block_tree)),
        block_executor_(std::move(block_executor)),
        router_(std::move(router)),
        scheduler_(std::move(scheduler)),
        hasher_(std::move(hasher)) {
    BOOST_ASSERT(block_tree_);
    BOOST_ASSERT(block_executor_);
    BOOST_ASSERT(router_);
    BOOST_ASSERT(scheduler_);
    BOOST_ASSERT(hasher_);

    BOOST_ASSERT(app_state_manager);
    app_state_manager->atShutdown([this] { node_is_shutting_down_ = true; });
  }

  void BabeSynchronizerImpl::syncByBlockInfo(
      const primitives::BlockInfo &block_info,
      const libp2p::peer::PeerId &peer_id,
      BabeSynchronizer::SyncResultHandler &&handler) {
    // If provided block is already enqueued, nothing to do additional
    if (isInQueue(block_info.hash)) {
      handler(block_info);
      return;
    }

    // We are communicating with one peer only for one issue.
    // If peer is already in use, don't start an additional issue.
    auto peer_is_busy = not busy_peers_.emplace(peer_id).second;
    if (peer_is_busy) {
      SL_TRACE(log_,
               "Can't syncByBlockHeader block #{} hash={} is received from {}: "
               "Peer busy",
               block_info.number,
               block_info.hash.toHex(),
               peer_id.toBase58());
      handler(Error::PEER_BUSY);
      return;
    }
    SL_TRACE(log_, "Peer {} marked as busy", peer_id.toBase58());

    const auto &last_finalized_block = block_tree_->getLastFinalized();

    auto best_block_res =
        block_tree_->getBestContaining(last_finalized_block.hash, boost::none);
    BOOST_ASSERT(best_block_res.has_value());
    const auto &best_block = best_block_res.value();

    // Provided block is equal our best one. Nothing needs to do.
    if (block_info == best_block) {
      handler(block_info);
      return;
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
    BabeSynchronizer::SyncResultHandler find_handler =
        [wp = weak_from_this(), peer_id, handler = std::move(handler)](
            outcome::result<primitives::BlockInfo> res) mutable {
          if (auto self = wp.lock()) {
            // Remove peer from list of busy peers
            if (self->busy_peers_.erase(peer_id) > 0) {
              SL_TRACE(
                  self->log_, "Peer {} unmarked as busy", peer_id.toBase58());
            }

            // Finding the best common block was failed
            if (not res.has_value()) {
              handler(res.as_failure());
              return;
            }

            // If provided block is already enqueued, nothing to do additional
            auto &block_info = res.value();
            if (self->isInQueue(block_info.hash)) {
              handler(std::move(block_info));
              return;
            }

            // Start to load blocks since found
            SL_DEBUG(self->log_,
                     "Start to load blocks from {} since block #{} hash={}",
                     peer_id.toBase58(),
                     block_info.number,
                     block_info.hash.toHex());
            self->loadBlocks(peer_id, block_info, std::move(handler));
          }
        };

    // Find the best common block
    SL_DEBUG(log_,
             "Start to find common block with {} in #{}..#{} to catch up",
             peer_id.toBase58(),
             lower,
             upper);
    findCommonBlock(peer_id, lower, upper, hint, std::move(find_handler));
  }

  void BabeSynchronizerImpl::syncByBlockHeader(
      const primitives::BlockHeader &header,
      const libp2p::peer::PeerId &peer_id,
      BabeSynchronizer::SyncResultHandler &&handler) {
    auto block_hash = hasher_->blake2b_256(scale::encode(header).value());
    const primitives::BlockInfo block_info(header.number, block_hash);

    // Block was applied before
    if (block_tree_->getBlockHeader(block_hash).has_value()) {
      return;
    }

    // Block is already enqueued
    if (isInQueue(block_info.hash)) {
      return;
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
      return;
    }

    // Otherwise is using base way to enqueue
    syncByBlockInfo(block_info, peer_id, [wp = weak_from_this()](auto res) {
      if (auto self = wp.lock()) {
        SL_TRACE(self->log_, "Block(s) enqueued to load by announce");
      }
    });
  }

  bool BabeSynchronizerImpl::isInQueue(
      const primitives::BlockHash &hash) const {
    return known_blocks_.find(hash) != known_blocks_.end();
  }

  void BabeSynchronizerImpl::findCommonBlock(
      const libp2p::peer::PeerId &peer_id,
      primitives::BlockNumber lower,
      primitives::BlockNumber upper,
      primitives::BlockNumber hint,
      SyncResultHandler &&handler) const {
    static std::random_device rd{};
    static std::uniform_int_distribution<primitives::BlocksRequestId> dis{};

    // Interrupts process if node is shutting down
    if (node_is_shutting_down_) {
      handler(Error::SHUTTING_DOWN);
      return;
    }

    auto response_handler = [wp = weak_from_this(),
                             lower,
                             upper,
                             target = hint,
                             peer_id,
                             handler = std::move(handler)](
                                auto &&response_res) mutable {
      auto self = wp.lock();
      if (not self) {
        return;
      }

      // Any error interrupts finding common block
      if (response_res.has_error()) {
        SL_ERROR(self->log_,
                 "Can't check if block #{} in #{}..#{} is common with {}: {}",
                 target,
                 lower,
                 upper,
                 peer_id.toBase58(),
                 response_res.error().message());
        handler(response_res.as_failure());
        return;
      }
      auto &blocks = response_res.value().blocks;

      // No block in response is abnormal situation. Requested block must be
      // existed because finding in interval of numbers of blocks what must
      // existing
      if (blocks.empty()) {
        SL_ERROR(self->log_,
                 "Can't check if block #{} in #{}..#{} is common with {}: "
                 "Response does not have any blocks",
                 target,
                 lower,
                 upper,
                 peer_id.toBase58());
        handler(Error::EMPTY_RESPONSE);
        return;
      }

      const primitives::BlockData &block = blocks.front();

      // Check if block is known (is already enqueued or is in block tree)
      bool block_is_known =
          self->known_blocks_.find(block.hash) != self->known_blocks_.end()
          or self->block_tree_->getBlockHeader(block.hash).has_value();

      // Interval of finding is totally narrowed. Common block should be found
      if (target == lower) {
        if (block_is_known) {
          // Common block is found
          SL_DEBUG(self->log_,
                   "Found best common block with {}: #{} hash={}",
                   peer_id.toBase58(),
                   target,
                   block.hash.toHex());
          handler(primitives::BlockInfo(target, block.hash));
          return;
        }

        // Common block is not found. It is abnormal situation. Requested
        // block must be existed because finding in interval of numbers of
        // blocks what must existing
        SL_WARN(self->log_,
                "Not found any common block with {}",
                peer_id.toBase58());
        handler(Error::EMPTY_RESPONSE);
        return;
      }

      // Step for next iteration
      auto step = upper - target + 1;

      // Narrowing interval for next iteration
      if (block_is_known) {
        SL_TRACE(self->log_,
                 "Found common block #{} with {} in #{}..#{}",
                 target,
                 peer_id.toBase58(),
                 lower,
                 upper);

        // Narrowing interval to continue above
        lower = target;
      } else {
        SL_TRACE(self->log_,
                 "Not found common block #{} with {} in #{}..#{}",
                 target,
                 peer_id.toBase58(),
                 lower,
                 upper);

        // Narrowing interval to continue below
        upper = target;
      }

      // Speed up of dive if possible or Bisect otherwise
      primitives::BlockNumber hint =
          lower + std::min(step, (upper - lower) / 2);

      // Try again with narrowed interval
      self->findCommonBlock(peer_id, lower, upper, hint, std::move(handler));
    };

    SL_TRACE(log_,
             "Check if block #{} in #{}..#{} is common with {}",
             hint,
             lower,
             upper,
             peer_id.toBase58());

    network::BlocksRequest request{dis(rd),
                                   // TODO: perhaps hash would be enough
                                   network::BlockAttribute::HEADER,
                                   hint,
                                   boost::none,
                                   network::Direction::ASCENDING,
                                   1};

    auto protocol = router_->getSyncProtocol();
    BOOST_ASSERT_MSG(protocol, "Router did not provide sync protocol");
    protocol->request(peer_id, std::move(request), std::move(response_handler));
  }

  void BabeSynchronizerImpl::loadBlocks(const libp2p::peer::PeerId &peer_id,
                                        primitives::BlockInfo from,
                                        SyncResultHandler &&handler) {
    static std::random_device rd{};
    static std::uniform_int_distribution<primitives::BlocksRequestId> dis{};

    // Interrupts process if node is shutting down
    if (node_is_shutting_down_) {
      handler(Error::SHUTTING_DOWN);
      return;
    }

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
                 "Can't load blocks from {} beggining block #{} hash={}: {}",
                 peer_id.toBase58(),
                 from.number,
                 from.hash,
                 response_res.error().message());
        handler(response_res.as_failure());
        return;
      }
      auto &blocks = response_res.value().blocks;

      // No block in response is abnormal situation.
      // At least one starting block should be returned as existing
      if (blocks.empty()) {
        SL_ERROR(self->log_,
                 "Can't load blocks from {} beggining block #{} hash={}: ",
                 "Response does not have any blocks",
                 peer_id.toBase58(),
                 from.number,
                 from.hash);
        handler(Error::EMPTY_RESPONSE);
        return;
      }

      SL_TRACE(self->log_,
               "{} blocks are loaded from {} beggining block #{} hash={}",
               blocks.size(),
               peer_id.toBase58(),
               from.number,
               from.hash);

      bool some_blocks_added = false;
      primitives::BlockInfo last_loaded_block;

      for (auto &block : blocks) {
        // Check if header is provided
        if (not block.header.has_value()) {
          SL_ERROR(self->log_,
                   "Can't load blocks from {} starting from block #{} hash={}: "
                   "Received block without header",
                   peer_id.toBase58(),
                   from.number,
                   from.hash);
          handler(Error::RESPONSE_WITHOUT_BLOCK_HEADER);
          return;
        }
        // Check if body is provided
        if (not block.header.has_value()) {
          SL_ERROR(self->log_,
                   "Can't load blocks from {} starting from block #{} hash={}: "
                   "Received block without body",
                   peer_id.toBase58(),
                   from.number,
                   from.hash);
          handler(Error::RESPONSE_WITHOUT_BLOCK_BODY);
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
                       "Can't load blocks from {} starting from block #{} "
                       "hash={}: "
                       "Received discarded block (#{} hash={})",
                       peer_id.toBase58(),
                       from.number,
                       from.hash.toHex(),
                       header.number,
                       block.hash);
              handler(Error::DISCARDED_BLOCK);
              return;
            }

            SL_TRACE(self->log_,
                     "Skip block #{} hash={} received from {}: "
                     "it is finalized",
                     header.number,
                     block.hash.toHex(),
                     peer_id.toBase58(),
                     last_finalized_block.number);
            continue;
          }

          SL_TRACE(self->log_,
                   "Skip block #{} hash={} received from {}: "
                   "it is below the last finalized (#{})",
                   header.number,
                   block.hash.toHex(),
                   peer_id.toBase58(),
                   last_finalized_block.number);
          continue;
        }

        // Check if block is not discarded
        if (last_finalized_block.number + 1 == header.number) {
          if (last_finalized_block.hash != header.parent_hash) {
            SL_ERROR(self->log_,
                     "Can't complete blocks loading from {} starting from "
                     "block #{} hash={}: "
                     "Received discarded block (#{} hash={})",
                     peer_id.toBase58(),
                     from.number,
                     from.hash.toHex(),
                     header.number,
                     header.parent_hash.toHex());
            handler(Error::DISCARDED_BLOCK);
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
                   "block #{} hash={}: "
                   "Received block is not descendant of previous",
                   peer_id.toBase58(),
                   from.number,
                   from.hash.toHex());
          handler(Error::WRONG_ORDER);
          return;
        }

        // Check if hash is valid
        auto calculated_hash =
            self->hasher_->blake2b_256(scale::encode(header).value());
        if (block.hash != calculated_hash) {
          SL_ERROR(self->log_,
                   "Can't complete blocks loading from {} starting from "
                   "block #{} hash={}: "
                   "Received block whose hash does not match the header",
                   peer_id.toBase58(),
                   from.number,
                   from.hash.toHex());
          handler(Error::INVALID_HASH);
          return;
        }

        last_loaded_block.number = header.number;
        last_loaded_block.hash = block.hash;

        parent_hash = block.hash;

        // Add block in queue and save peer or just add peer for existing record
        auto it = self->known_blocks_.find(block.hash);
        if (it == self->known_blocks_.end()) {
          self->known_blocks_.emplace(block.hash, KnownBlock{block, {peer_id}});
        } else {
          it->second.peers.emplace(peer_id);
          SL_TRACE(self->log_,
                   "Skip block #{} hash={} received from {}: "
                   "already enqueued",
                   header.number,
                   block.hash.toHex(),
                   peer_id.toBase58());
          continue;
        }

        SL_TRACE(self->log_,
                 "Enqueue block #{} hash={} received from {}",
                 header.number,
                 block.hash.toHex(),
                 peer_id.toBase58());

        self->generations_.emplace(header.number, block.hash);
        self->ancestry_.emplace(header.parent_hash, block.hash);

        some_blocks_added = true;
      }

      SL_TRACE(self->log_, "Block loading is finished");
      handler(last_loaded_block);

      if (some_blocks_added) {
        SL_TRACE(self->log_, "Enqueued some new blocks: schedule applying");
        self->scheduler_->schedule([wp] {
          if (auto self = wp.lock()) {
            self->applyNextBlock();
          }
        });
      }
    };

    network::BlocksRequest request{dis(rd),
                                   network::BlockAttribute::HEADER
                                       | network::BlockAttribute::BODY
                                       | network::BlockAttribute::JUSTIFICATION,
                                   from.hash,
                                   boost::none,
                                   network::Direction::ASCENDING,
                                   boost::none};

    auto protocol = router_->getSyncProtocol();
    BOOST_ASSERT_MSG(protocol, "Router did not provide sync protocol");
    protocol->request(peer_id, std::move(request), std::move(response_handler));
  }

  void BabeSynchronizerImpl::applyNextBlock() {
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
              "Block #{} hash={} {} not applied as discarded",
              number,
              hash.toHex(),
              n ? fmt::format("and {} others have", n) : fmt::format("has"));
          if (handler) handler(Error::DISCARDED_BLOCK);
        }
      } else {
        auto applying_res = block_executor_->applyBlock(std::move(block));
        if (not applying_res.has_value()) {
          if (applying_res
              != outcome::failure(blockchain::BlockTreeError::BLOCK_EXISTS)) {
            auto n = discardBlock(block.hash);
            SL_WARN(
                log_,
                "Block #{} hash={} {} was discarded: {}",
                number,
                hash.toHex(),
                n ? fmt::format("and {} others have", n) : fmt::format("has"),
                applying_res.error().message());
            if (handler) handler(Error::DISCARDED_BLOCK);
          } else {
            SL_DEBUG(log_,
                     "Block #{} hash={} is skipped as existing",
                     block.header->number,
                     hash.toHex());
            if (handler) handler(applying_res.as_failure());
          }
        } else {
          if (handler) handler(grandpa::BlockInfo(block.header->number, hash));
        }
      }
    }
    ancestry_.erase(hash);

    if (known_blocks_.size() < kMinPreloadedBlockNumber) {
      SL_TRACE(log_,
               "{} blocks in queue: ask next portion of block",
               known_blocks_.size());
      askNextPortionOfBlocks();
    } else {
      SL_TRACE(log_, "{} blocks in queue", known_blocks_.size());
    }

    scheduler_->schedule([wp = weak_from_this()] {
      if (auto self = wp.lock()) {
        self->applyNextBlock();
      }
    });
  }

  size_t BabeSynchronizerImpl::discardBlock(
      const primitives::BlockHash &hash_of_discarding_block) {
    std::queue<primitives::BlockHash> queue;
    queue.emplace(hash_of_discarding_block);

    size_t affected = 0;
    while (not queue.empty()) {
      const auto &hash = queue.front();

      affected += known_blocks_.erase(hash);

      auto range = ancestry_.equal_range(hash);
      for (auto it = range.first; it != range.second; ++it) {
        queue.emplace(it->second);
      }
      ancestry_.erase(range.first, range.second);

      queue.pop();
    }

    return affected;
  }

  void BabeSynchronizerImpl::prune(
      const primitives::BlockInfo &finalized_block) {
    // Remove blocks whose numbers less finalized one
    while (not generations_.empty()) {
      auto generation_node = generations_.extract(generations_.begin());
      if (generation_node) {
        const auto &number = generation_node.key();
        if (number >= finalized_block.number) {
          break;
        }
        const auto &hash = generation_node.mapped();
        known_blocks_.erase(hash);
        ancestry_.erase(hash);
      }
    }

    // Remove blocks whose numbers equal finalized one, exceptly finalized one
    auto range = generations_.equal_range(finalized_block.number);
    for (auto it = range.first; it != range.second;) {
      auto cit = it++;
      const auto &hash = cit->second;
      if (hash != finalized_block.hash) {
        discardBlock(hash);
      }
    }
  }

  void BabeSynchronizerImpl::askNextPortionOfBlocks() {
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
                 "Block #{} hash={} is unknown. Go to next one",
                 g_it->first,
                 hash.toHex());
        continue;
      }

      auto &peers = b_it->second.peers;
      if (peers.empty()) {
        SL_TRACE(log_,
                 "Block #{} hash={} don't have any peer. Go to next one",
                 g_it->first,
                 hash.toHex());
        continue;
      }

      for (auto p_it = peers.begin(); p_it != peers.end();) {
        auto cp_it = p_it++;

        auto peer_id = *cp_it;

        if (busy_peers_.find(peer_id) != busy_peers_.end()) {
          SL_TRACE(log_,
                   "Peer {} for block #{} hash={} is busy",
                   peer_id.toBase58(),
                   g_it->first,
                   hash.toHex());
          continue;
        }

        busy_peers_.insert(peers.extract(cp_it));
        SL_TRACE(log_, "Peer {} marked as busy", peer_id.toBase58());

        auto handler = [wp = weak_from_this(), peer_id](const auto &res) {
          if (auto self = wp.lock()) {
            if (self->busy_peers_.erase(peer_id) > 0) {
              SL_TRACE(
                  self->log_, "Peer {} unmarked as busy", peer_id.toBase58());
            }
            SL_TRACE(self->log_, "End asking portion of blocks");
            self->asking_blocks_portion_in_progress_ = false;
            if (not res.has_value()) {
              SL_DEBUG(self->log_,
                       "Loading next portion of blocks from {} is failed: {}",
                       peer_id.toBase58(),
                       res.error().message());
              return;
            }
            SL_DEBUG(self->log_,
                     "Portion of blocks from {} is loaded",
                     peer_id.toBase58());
          }
        };

        auto lower = generations_.begin()->first;
        auto upper = generations_.rbegin()->first + 1;
        auto hint = generations_.rbegin()->first;

        SL_DEBUG(log_,
                 "Start to find common block with {} in #{}..#{} to fill queue",
                 peer_id.toBase58(),
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
                           peer_id.toBase58(),
                           res.error().message());
                  handler(res);
                  return;
                }
                auto &block_info = res.value();
                SL_DEBUG(self->log_,
                         "Start to load next portion of blocks from {} "
                         "since block #{} hash={}",
                         peer_id.toBase58(),
                         block_info.number,
                         block_info.hash.toHex());
                self->loadBlocks(peer_id, block_info, std::move(handler));
              }
            });
        return;
      }

      SL_TRACE(
          log_,
          "Block #{} hash={} doesn't have appropriate peer. Go to next one",
          g_it->first,
          hash.toHex());
    }

    SL_TRACE(log_, "End asking portion of blocks: none");
    asking_blocks_portion_in_progress_ = false;
  }

}  // namespace kagome::consensus
