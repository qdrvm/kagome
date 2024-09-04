/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/verified_justification_queue.hpp"
#include "application/app_state_manager.hpp"
#include "blockchain/block_tree.hpp"
#include "common/main_thread_pool.hpp"
#include "consensus/grandpa/authority_manager.hpp"
#include "consensus/grandpa/has_authority_set_change.hpp"
#include "consensus/timeline/timeline.hpp"
#include "network/synchronizer.hpp"
#include "utils/pool_handler.hpp"

namespace kagome::consensus::grandpa {
  /// When to start fetching justification range
  constexpr size_t kRangeStart = 8;

  VerifiedJustificationQueue::VerifiedJustificationQueue(
      application::AppStateManager &app_state_manager,
      common::MainThreadPool &main_thread_pool,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<AuthorityManager> authority_manager,
      LazySPtr<network::Synchronizer> synchronizer,
      LazySPtr<Timeline> timeline,
      primitives::events::ChainSubscriptionEnginePtr chain_sub_engine)
      : main_pool_handler_{main_thread_pool.handler(app_state_manager)},
        block_tree_{std::move(block_tree)},
        authority_manager_{std::move(authority_manager)},
        synchronizer_{synchronizer},
        timeline_{timeline},
        chain_sub_{chain_sub_engine},
        log_{log::createLogger("VerifiedJustificationQueue", "grandpa")} {
    BOOST_ASSERT(main_pool_handler_ != nullptr);
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(authority_manager_ != nullptr);

    app_state_manager.takeControl(*this);
  }

  void VerifiedJustificationQueue::start() {
    if (auto r = authority_manager_->authorities(
            block_tree_->getLastFinalized(), true)) {
      expected_ = (**r).id;
    }
    chain_sub_.onHead([weak{weak_from_this()}]() {
      if (auto self = weak.lock()) {
        self->possibleLoop();
      }
    });
  }

  void VerifiedJustificationQueue::addVerified(
      AuthoritySetId set, GrandpaJustification justification) {
    REINVOKE(*main_pool_handler_, addVerified, set, std::move(justification));
    if (set < expected_) {
      return;
    }
    if (justification.block_info.number
        <= block_tree_->getLastFinalized().number) {
      return;
    }
    auto block_res = block_tree_->getBlockHeader(justification.block_info.hash);
    if (not block_res) {
      return;
    }
    auto &block = block_res.value();
    consensus::grandpa::HasAuthoritySetChange digest{block};
    required_.erase(justification.block_info);
    auto ready = [&] {
      if (not digest.scheduled) {
        finalize(std::nullopt, justification);
        return;
      }
      finalize(set, justification);
      verifiedLoop();
      requiredLoop();
      possibleLoop();
    };
    if (set == expected_) {
      ready();
      return;
    }
    auto parent_res =
        authority_manager_->scheduledParent(justification.block_info);
    if (not parent_res) {
      return;
    }
    auto &parent = parent_res.value();
    auto expected = parent.second + 1;
    if (expected == expected_) {
      ready();
      return;
    }
    while (parent.second >= expected_) {
      if (not verified_.contains(parent.second)) {
        if (required_.emplace(parent.first).second) {
          SL_INFO(log_,
                  "missing justification for block {} set {}",
                  parent.first,
                  parent.second);
        }
      }
      auto parent_res = authority_manager_->scheduledParent(parent.first);
      if (not parent_res) {
        break;
      }
      parent = parent_res.value();
    }
    requiredLoop();
    if (not digest.scheduled) {
      if (not last_
          or justification.block_info.number
                 > last_->second.block_info.number) {
        last_.emplace(set, std::move(justification));
      }
      return;
    }
    verified_.emplace(set, std::make_pair(expected, std::move(justification)));
  }

  void VerifiedJustificationQueue::finalize(
      std::optional<AuthoritySetId> set,
      const consensus::grandpa::GrandpaJustification &justification) {
    if (auto r = block_tree_->finalize(
            justification.block_info.hash,
            {common::Buffer{scale::encode(justification).value()}});
        not r) {
      return;
    }
    if (set) {
      expected_ = *set + 1;
    }
    possible_.clear();
  }

  void VerifiedJustificationQueue::verifiedLoop() {
    while (not verified_.empty()) {
      auto &[set, p] = *verified_.begin();
      auto &[expected, justification] = p;
      if (expected > expected_) {
        break;
      }
      if (expected == expected_) {
        finalize(set, justification);
      }
      verified_.erase(verified_.begin());
    }
    if (last_) {
      auto &[set, justification] = *last_;
      if (set < expected_) {
        last_.reset();
      } else if (set == expected_) {
        finalize(std::nullopt, justification);
        last_.reset();
      }
    }
  }

  void VerifiedJustificationQueue::requiredLoop() {
    if (fetching_ or required_.empty()) {
      return;
    }
    auto block = *required_.begin();
    auto cb = [weak{weak_from_this()}, block](outcome::result<void> r) {
      auto self = weak.lock();
      if (not self) {
        return;
      }
      self->fetching_ = false;
      if (r) {
        self->required_.erase(block);
      }
      self->requiredLoop();
    };
    fetching_ = synchronizer_.get()->fetchJustification(block, std::move(cb));
  }

  void VerifiedJustificationQueue::possibleLoop() {
    if (fetching_ or not required_.empty()) {
      return;
    }
    if (not verified_.empty() or last_) {
      return;
    }
    if (possible_.empty()) {
      possible_ = authority_manager_->possibleScheduled();
    }
    if (possible_.empty()) {
      rangeLoop();
      return;
    }
    auto block = possible_.back();
    possible_.pop_back();
    auto cb = [weak{weak_from_this()}, block](outcome::result<void> r) {
      auto self = weak.lock();
      if (not self) {
        return;
      }
      self->fetching_ = false;
      self->requiredLoop();
      if (not r) {
        self->possibleLoop();
      } else {
        SL_INFO(
            self->log_, "possible justification for block {}", block.number);
      }
    };
    fetching_ = synchronizer_.get()->fetchJustification(block, std::move(cb));
  }

  void VerifiedJustificationQueue::rangeLoop() {
    if (not timeline_.get()->wasSynchronized()) {
      return;
    }
    auto finalized = block_tree_->getLastFinalized().number;
    auto best = block_tree_->bestBlock().number;
    if (best - finalized < kRangeStart) {
      return;
    }
    range_ = std::max(range_, finalized + 1);
    if (range_ > best) {
      range_ = 0;
      return;
    }
    auto cb = [weak{weak_from_this()}](
                  outcome::result<std::optional<primitives::BlockNumber>> r) {
      auto self = weak.lock();
      if (not self) {
        return;
      }
      self->fetching_ = false;
      if (r) {
        if (auto &next = r.value()) {
          self->range_ = *next;
        } else {
          SL_INFO(self->log_, "justification for range {}..", self->range_);
        }
      }
      self->requiredLoop();
      self->possibleLoop();
    };
    fetching_ =
        synchronizer_.get()->fetchJustificationRange(range_, std::move(cb));
    if (fetching_) {
      SL_INFO(log_, "fething justification range {}..", range_);
    }
  }

  void VerifiedJustificationQueue::warp() {
    if (auto r = authority_manager_->authorities(
            block_tree_->getLastFinalized(), true)) {
      expected_ = (**r).id;
    }
    required_.clear();
  }
}  // namespace kagome::consensus::grandpa
