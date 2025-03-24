/**
 * Copyright Quadrivium LLC
 * All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include "authority_manager_impl.hpp"

#include "application/app_state_manager.hpp"
#include "blockchain/block_tree.hpp"
#include "consensus/grandpa/authority_manager_error.hpp"
#include "consensus/grandpa/impl/kusama_hard_forks.hpp"
#include "consensus/grandpa/types/scheduled_change.hpp"
#include "runtime/runtime_api/grandpa_api.hpp"
#include "storage/map_prefix/prefix.hpp"
#include "storage/predefined_keys.hpp"

using kagome::common::Buffer;

namespace kagome::consensus::grandpa {

  AuthorityManagerImpl::AuthorityManagerImpl(
      std::shared_ptr<application::AppStateManager> app_state_manager,
      std::shared_ptr<blockchain::BlockTree> block_tree,
      std::shared_ptr<runtime::GrandpaApi> grandpa_api,
      std::shared_ptr<storage::SpacedStorage> persistent_storage,
      primitives::events::ChainSubscriptionEnginePtr chain_events_engine)
      : block_tree_(std::move(block_tree)),
        grandpa_api_(std::move(grandpa_api)),
        persistent_storage_{
            persistent_storage->getSpace(storage::Space::kDefault)},
        chain_sub_{std::move(chain_events_engine)},
        indexer_{
            std::make_shared<storage::MapPrefix>(
                storage::kAuthorityManagerImplIndexerPrefix,
                persistent_storage_),
            block_tree_,
        },
        logger_{log::createLogger("AuthorityManager", "authority")} {
    BOOST_ASSERT(block_tree_ != nullptr);
    BOOST_ASSERT(grandpa_api_ != nullptr);
    BOOST_ASSERT(persistent_storage_ != nullptr);

    if (auto r = indexer_.init(); not r) {
      logger_->error("Indexer::init error: {}", r.error());
    }

    BOOST_ASSERT(app_state_manager != nullptr);
    app_state_manager->takeControl(*this);
  }

  bool AuthorityManagerImpl::prepare() {
    chain_sub_.onFinalize(
        [weak{weak_from_this()}](const primitives::BlockHeader &block) {
          if (auto self = weak.lock()) {
            std::unique_lock lock{self->mutex_};
            self->indexer_.finalize();
            // TODO(turuslan): #1854, rebase ambigous forced changes
          }
        });

    return true;
  }

  std::optional<std::shared_ptr<const AuthoritySet>>
  AuthorityManagerImpl::authorities(const primitives::BlockInfo &target_block,
                                    IsBlockFinalized finalized) const {
    if (not block_tree_->has(target_block.hash)) {
      return std::nullopt;
    }
    std::unique_lock lock{mutex_};
    auto res = authoritiesOutcome(target_block, finalized.operator bool());
    if (res.has_value()) {
      return std::move(res.value());
    }

    SL_WARN(logger_,
            "authorities {} finalized={} error: {}",
            target_block,
            (bool)finalized,
            res.error());
    return std::nullopt;
  }

  outcome::result<std::shared_ptr<const AuthoritySet>>
  AuthorityManagerImpl::authoritiesOutcome(const primitives::BlockInfo &block,
                                           bool next) const {
    auto descent = indexer_.startDescentFrom(block);
    outcome::result<void> cb_res = outcome::success();
    auto cb = [&](std::optional<primitives::BlockInfo> prev,
                  size_t i_first,
                  size_t i_last) {
      cb_res = [&]() -> outcome::result<void> {
        BOOST_ASSERT(i_first >= i_last);
        auto info = descent.path_.at(i_first);
        std::shared_ptr<const AuthoritySet> prev_state;
        [[unlikely]] if (info.number == 0) {
          OUTCOME_TRY(list, grandpa_api_->authorities(info.hash));
          auto genesis = std::make_shared<AuthoritySet>(0, std::move(list));
          GrandpaIndexedValue value{
              .next_set_id = genesis->id,
              .state = genesis,
              .next = genesis,
          };
          indexer_.put(info,
                       {
                           .value = value,
                           .prev = std::nullopt,
                       },
                       true);
          if (i_first == i_last) {
            return outcome::success();
          }
          prev = info;
          prev_state = genesis;
          --i_first;
        }
        if (not prev_state) {
          BOOST_OUTCOME_TRY(prev_state, loadPrev(prev));
        }
        while (true) {
          info = descent.path_.at(i_first);
          OUTCOME_TRY(header, block_tree_->getBlockHeader(info.hash));
          HasAuthoritySetChange digests{header};
          if (digests.forced and digests.forced->delay_start >= info.number) {
            SL_WARN(logger_,
                    "ForcedChange on {} ignored, targets future block {}",
                    info,
                    digests.forced->delay_start);
            digests.scheduled.reset();
            digests.forced.reset();
          }
          if (digests) {
            GrandpaIndexedValue value;
            if (digests.forced) {
              if (not prev) {
                return AuthorityManagerError::PREVIOUS_NOT_FOUND;
              }
              while (true) {
                auto res = indexer_.get(*prev);
                if (not res or not res->value) {
                  return AuthorityManagerError::PREVIOUS_NOT_FOUND;
                }
                if (prev->number <= digests.forced->delay_start
                    or res->value->forced_target or res->value->state
                    or block_tree_->getBlockJustification(prev->hash)) {
                  value.next_set_id = res->value->next_set_id + 1;
                  value.forced_target =
                      std::max(digests.forced->delay_start, prev->number);
                  break;
                }
                if (not res->prev) {
                  return AuthorityManagerError::PREVIOUS_NOT_FOUND;
                }
                prev = res->prev;
              }
            } else {
              value.next_set_id = prev_state->id + 1;
            }
            auto state = applyDigests(info, value.next_set_id, digests);
            value.next = state;
            indexer_.put(info,
                         {.value = value, .prev = prev},
                         block_tree_->isFinalized(info));
            prev = info;
            prev_state = state;
          } else {
            indexer_.put(info,
                         {.value = std::nullopt, .prev = prev, .inherit = true},
                         false);
          }
          if (i_first == i_last) {
            break;
          }
          --i_first;
        }
        return outcome::success();
      }();
    };
    auto r = indexer_.search(descent, block, cb);
    OUTCOME_TRY(cb_res);
    if (not r) {
      return AuthorityManagerError::NOT_FOUND;
    }
    if (r->second.value->state) {
      return *r->second.value->state;
    }
    if (next or r->second.value->forced_target or r->first != block) {
      OUTCOME_TRY(load(r->first, r->second));
      return *r->second.value->next;
    }
    return loadPrev(r->second.prev);
  }

  std::shared_ptr<AuthoritySet> AuthorityManagerImpl::applyDigests(
      const primitives::BlockInfo &block,
      AuthoritySetId set_id,
      const HasAuthoritySetChange &digests) const {
    BOOST_ASSERT(digests);
    return std::make_shared<AuthoritySet>(
        set_id,
        isKusamaHardFork(block_tree_->getGenesisBlockHash(), block)
            ? kusamaHardForksAuthorities()
        : digests.forced ? digests.forced->authorities
                         : digests.scheduled->authorities);
  }

  outcome::result<void> AuthorityManagerImpl::load(
      const primitives::BlockInfo &block,
      blockchain::Indexed<GrandpaIndexedValue> &item) const {
    if (not item.value->next) {
      if (item.value->state) {
        item.value->next = item.value->state;
      } else {
        BOOST_ASSERT(block.number != 0);
        OUTCOME_TRY(header, block_tree_->getBlockHeader(block.hash));
        item.value->next =
            applyDigests(block, item.value->next_set_id, {header});
        indexer_.put(block, item, false);
      }
    }
    return outcome::success();
  }

  outcome::result<std::shared_ptr<const AuthoritySet>>
  AuthorityManagerImpl::loadPrev(
      const std::optional<primitives::BlockInfo> &prev) const {
    if (not prev) {
      return AuthorityManagerError::PREVIOUS_NOT_FOUND;
    }
    auto r = indexer_.get(*prev);
    if (not r or not r->value) {
      return AuthorityManagerError::PREVIOUS_NOT_FOUND;
    }
    OUTCOME_TRY(load(*prev, *r));
    return *r->value->next;
  }

  void AuthorityManagerImpl::warp(const primitives::BlockInfo &block,
                                  const primitives::BlockHeader &header,
                                  const AuthoritySet &authorities) {
    std::unique_lock lock{mutex_};
    GrandpaIndexedValue value{
        .next_set_id = authorities.id + 1,
    };
    HasAuthoritySetChange digests{header};
    if (not digests.scheduled) {
      auto state = std::make_shared<AuthoritySet>(authorities);
      value = {.next_set_id = authorities.id, .state = state, .next = state};
    }
    indexer_.put(block, {.value = value}, true);
  }

  AuthorityManager::ScheduledParentResult AuthorityManagerImpl::scheduledParent(
      primitives::BlockInfo block) const {
    std::unique_lock lock{mutex_};
    OUTCOME_TRY(authoritiesOutcome(block, true));
    auto skip = true;
    while (true) {
      auto r = indexer_.get(block);
      if (not r) {
        break;
      }
      if (not skip and not r->inherit) {
        if (not r->value) {
          break;
        }
        if (r->value->state) {
          break;
        }
        if (not r->value->forced_target) {
          return std::make_pair(block, r->value->next_set_id - 1);
        }
      } else {
        skip = false;
      }
      if (not r->prev) {
        break;
      }
      block = *r->prev;
    }
    return AuthorityManagerError::NOT_FOUND;
  }

  std::vector<primitives::BlockInfo> AuthorityManagerImpl::possibleScheduled()
      const {
    std::unique_lock lock{mutex_};
    for (auto &hash : block_tree_->getLeaves()) {
      if (auto r = block_tree_->getBlockHeader(hash)) {
        auto &block = r.value();
        std::ignore = authoritiesOutcome({block.number, hash}, true);
      }
    }
    std::vector<primitives::BlockInfo> possible;
    auto finalized = block_tree_->getLastFinalized();
    auto last = finalized;
    auto r = indexer_.get(last);
    if (not r) {
      return possible;
    }
    if (r->inherit) {
      if (not r->prev) {
        return possible;
      }
      last = *r->prev;
      r = indexer_.get(last);
      if (not r) {
        return possible;
      }
    }
    for (auto it = indexer_.map_.upper_bound(finalized);
         it != indexer_.map_.end();
         ++it) {
      if (not it->second.inherit and it->second.prev == last) {
        possible.emplace_back(it->first);
      }
    }
    return possible;
  }
}  // namespace kagome::consensus::grandpa
