/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "consensus/grandpa/impl/verified_justification_queue.hpp"
#include "application/app_state_manager.hpp"
#include "blockchain/block_tree.hpp"
#include "consensus/grandpa/authority_manager.hpp"
#include "consensus/grandpa/has_authority_set_change.hpp"
#include "network/synchronizer.hpp"
#include "utils/weak_io_context_post.hpp"

namespace kagome::consensus::grandpa {
  VerifiedJustificationQueue::VerifiedJustificationQueue(
      application::AppStateManager &app_state_manager,
      WeakIoContext main_thread,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<AuthorityManager> authority_manager,
      LazySPtr<network::Synchronizer> synchronizer,
      primitives::events::ChainSubscriptionEnginePtr chain_sub_engine)
      : main_thread_{std::move(main_thread)},
        block_tree_{std::move(block_tree)},
        authority_manager_{std::move(authority_manager)},
        synchronizer_{std::move(synchronizer)},
        chain_sub_{chain_sub_engine},
        log_{log::createLogger("VerifiedJustificationQueue")} {
    app_state_manager.takeControl(*this);
  }

  bool VerifiedJustificationQueue::start() {
    if (auto r = authority_manager_->authorities(
            block_tree_->getLastFinalized(), true)) {
      expected_ = (**r).id;
    }
    chain_sub_.onHead([weak{weak_from_this()}]() {
      if (auto self = weak.lock()) {
        self->possibleLoop();
      }
    });
    return true;
  }

  void VerifiedJustificationQueue::addVerified(
      AuthoritySetId set, GrandpaJustification justification) {
    REINVOKE(main_thread_, addVerified, set, std::move(justification));
    if (set < expected_) {
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
      fetchLoop();
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
    fetchLoop();
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

  void VerifiedJustificationQueue::fetchLoop() {
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
      self->fetchLoop();
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
      if (not self->required_.empty()) {
        self->fetchLoop();
      }
      if (not r) {
        self->possibleLoop();
      } else {
        SL_INFO(
            self->log_, "possible justification for block {}", block.number);
      }
    };
    fetching_ = synchronizer_.get()->fetchJustification(block, std::move(cb));
  }

  void VerifiedJustificationQueue::warp() {
    if (auto r = authority_manager_->authorities(
            block_tree_->getLastFinalized(), true)) {
      expected_ = (**r).id;
    }
    required_.clear();
  }
}  // namespace kagome::consensus::grandpa
